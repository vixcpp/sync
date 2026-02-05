/**
 *
 *  @file inflight_timeout.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp
 *
 */
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_http_transport.hpp"

static std::int64_t now_ms()
{
  using namespace std::chrono;
  return duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void reset_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static int run_inflight_timeout()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path dir = "./.vix_example_sync_inflight";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "example-sync-inflight"},
      store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      []
      { return true; });

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});

  SyncEngine::Config ecfg;
  ecfg.worker_count = 1;
  ecfg.batch_limit = 10;
  ecfg.idle_sleep_ms = 0;
  ecfg.offline_sleep_ms = 0;
  ecfg.inflight_timeout_ms = 50;

  SyncEngine engine(ecfg, outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const bool claimed = outbox->claim(id, t0);
  assert(claimed);

  {
    const auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::InFlight);
  }

  const auto t1 = t0 + 60;
  engine.tick(t1);

  {
    const auto saved = store->get(id);
    assert(saved.has_value());
    assert(saved->status != OperationStatus::InFlight);
    assert(saved->status == OperationStatus::Failed || saved->status == OperationStatus::Done);
  }

  engine.tick(t1 + 1);

  const auto final = store->get(id);
  assert(final.has_value());
  assert(final->status == OperationStatus::Done);

  assert(transport->callCount() >= 1);

  std::cout << "[sync] OK: inflight timeout requeues stuck ops and they complete\n";
  return 0;
}

int main()
{
  return run_inflight_timeout();
}
