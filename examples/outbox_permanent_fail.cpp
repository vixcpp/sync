/**
 *
 *  @file outbox_permanent_fail.cpp
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

static int run_permanent_fail()
{
  using namespace vix::sync;
  using namespace vix::sync::outbox;
  using namespace vix::sync::engine;

  const std::filesystem::path dir = "./.vix_example_sync_permfail";
  reset_dir(dir);

  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  auto outbox = std::make_shared<Outbox>(
      Outbox::Config{.owner = "example-sync-permfail"},
      store);

  auto probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      []
      { return true; });

  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setRuleForTarget(
      "/api/messages",
      FakeHttpTransport::Rule{
          .ok = false,
          .retryable = false,
          .error = "bad request (permanent)"});

  SyncEngine engine(
      SyncEngine::Config{.worker_count = 1, .batch_limit = 10, .idle_sleep_ms = 0, .offline_sleep_ms = 0},
      outbox, probe, transport);

  Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";

  const auto t0 = now_ms();
  const auto id = outbox->enqueue(op, t0);

  const auto processed1 = engine.tick(now_ms());
  assert(processed1 >= 1);
  assert(transport->callCount() == 1);

  const auto saved1 = store->get(id);
  assert(saved1.has_value());
  assert(saved1->status == OperationStatus::PermanentFailed);
  assert(saved1->last_error.find("permanent") != std::string::npos);

  const auto processed2 = engine.tick(now_ms());
  (void)processed2;
  assert(transport->callCount() == 1);

  const auto saved2 = store->get(id);
  assert(saved2.has_value());
  assert(saved2->status == OperationStatus::PermanentFailed);

  std::cout << "[sync] OK: permanent failure not retried, status PermanentFailed\n";
  return 0;
}

int main()
{
  return run_permanent_fail();
}
