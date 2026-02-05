/**
 *
 *  @file outbox_smoke.cpp
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

static void ensure_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::create_directories(dir, ec);
}

static int run_smoke()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path dir = "./.vix_example_sync_smoke";
  ensure_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "example-sync-smoke"},
      store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      []
      { return true; });

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});

  SyncEngine engine(
      SyncEngine::Config{.worker_count = 1, .batch_limit = 10},
      outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const auto processed = engine.tick(now_ms());
  assert(processed >= 1);

  const auto saved = store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "[sync] OK: smoke example sent and marked Done\n";
  return 0;
}

int main()
{
  return run_smoke();
}
