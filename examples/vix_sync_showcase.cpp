/**
 *
 *  @file vix_sync_showcase.cpp
 *  @author Gaspard Kirira
 *
 *  Vix.cpp - Sync Showcase (examples/)
 *
 *  Goal:
 *    A single, large, self-contained file that showcases how offline-first sync looks in Vix.cpp:
 *      - enqueue operations (local writes)
 *      - network probe online/offline
 *      - retryable failures
 *      - permanent failures (no retry)
 *      - inflight timeout sweep
 *      - batching behavior
 *      - “patterns” you can copy/paste for real projects
 *
 *  Notes:
 *    - This file focuses on public Sync API usage, not implementation details.
 *    - main() contains no business logic: it only calls functions.
 *
 *  Vix.cpp
 *
 */
// ============================================================================
// QUICK MAP
// ----------------------------------------------------------------------------
// 1) smoke_success()             -> enqueue + tick => Done
// 2) offline_no_send()           -> offline => no send, stays Pending
// 3) retryable_then_success()    -> fails retryable then succeeds
// 4) permanent_fail_no_retry()   -> PermanentFailed and stops
// 5) inflight_timeout_requeue()  -> InFlight stuck => sweep => retry => Done
// ============================================================================

#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>

#include "fake_transport.hpp"

using namespace vix::sync;
using namespace vix::sync::outbox;
using namespace vix::sync::engine;

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

struct SyncFixture
{
  std::filesystem::path dir;
  std::shared_ptr<FileOutboxStore> store;
  std::shared_ptr<Outbox> outbox;
  std::shared_ptr<vix::net::NetworkProbe> probe;
  std::shared_ptr<FakeTransport> transport;
  std::unique_ptr<SyncEngine> engine;

  bool online{true};
};

static SyncFixture make_fixture(std::string name, SyncEngine::Config cfg)
{
  SyncFixture f;
  f.dir = "./.vix_example_sync_" + name;
  reset_dir(f.dir);

  f.store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = f.dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  f.outbox = std::make_shared<Outbox>(Outbox::Config{.owner = "example-sync-" + name}, f.store);

  f.probe = std::make_shared<vix::net::NetworkProbe>(
      vix::net::NetworkProbe::Config{},
      [&f]
      { return f.online; });

  f.transport = std::make_shared<FakeTransport>();
  f.transport->setDefault({.ok = true});

  f.engine = std::make_unique<SyncEngine>(cfg, f.outbox, f.probe, f.transport);
  return f;
}

static Operation make_op(std::string kind, std::string target, std::string payload)
{
  Operation op;
  op.kind = std::move(kind);
  op.target = std::move(target);
  op.payload = std::move(payload);
  return op;
}

static void smoke_success()
{
  auto f = make_fixture("smoke", SyncEngine::Config{.worker_count = 1, .batch_limit = 10});

  const auto t0 = now_ms();
  const auto id = f.outbox->enqueue(make_op("http.post", "/api/messages", R"({"text":"hello"})"), t0);

  const auto processed = f.engine->tick(now_ms());
  assert(processed >= 1);

  const auto saved = f.store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "[1] OK: smoke_success\n";
}

static void offline_no_send()
{
  auto f = make_fixture("offline", SyncEngine::Config{.worker_count = 1, .batch_limit = 10, .idle_sleep_ms = 0, .offline_sleep_ms = 0});

  f.online = false;

  const auto t0 = now_ms();
  const auto id = f.outbox->enqueue(make_op("http.post", "/api/messages", R"({"text":"queued offline"})"), t0);

  const auto processed = f.engine->tick(now_ms());
  (void)processed;

  assert(f.transport->callCount() == 0);

  const auto saved = f.store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Pending);

  std::cout << "[2] OK: offline_no_send\n";
}

static void retryable_then_success()
{
  auto f = make_fixture("retry", SyncEngine::Config{.worker_count = 1, .batch_limit = 10, .idle_sleep_ms = 0, .offline_sleep_ms = 0});

  // First attempt fails retryable for this target
  f.transport->setRuleForTarget("/api/messages", FakeTransport::Rule{.ok = false, .retryable = true, .error = "temporary 503"});
  const auto t0 = now_ms();
  const auto id = f.outbox->enqueue(make_op("http.post", "/api/messages", R"({"text":"retry me"})"), t0);

  const auto processed1 = f.engine->tick(now_ms());
  assert(processed1 >= 1);
  assert(f.transport->callCount() == 1);

  // Now allow success
  f.transport->setRuleForTarget("/api/messages", FakeTransport::Rule{.ok = true});

  const auto processed2 = f.engine->tick(now_ms());
  assert(processed2 >= 1);
  assert(f.transport->callCount() >= 2);

  const auto saved = f.store->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);

  std::cout << "[3] OK: retryable_then_success\n";
}

static void permanent_fail_no_retry()
{
  auto f = make_fixture("permfail", SyncEngine::Config{.worker_count = 1, .batch_limit = 10, .idle_sleep_ms = 0, .offline_sleep_ms = 0});

  f.transport->setRuleForTarget("/api/messages", FakeTransport::Rule{
                                                     .ok = false,
                                                     .retryable = false,
                                                     .error = "bad request (permanent)"});

  const auto t0 = now_ms();
  const auto id = f.outbox->enqueue(make_op("http.post", "/api/messages", R"({"text":"bad payload"})"), t0);

  const auto processed1 = f.engine->tick(now_ms());
  assert(processed1 >= 1);
  assert(f.transport->callCount() == 1);

  const auto saved1 = f.store->get(id);
  assert(saved1.has_value());
  assert(saved1->status == OperationStatus::PermanentFailed);

  const auto processed2 = f.engine->tick(now_ms());
  (void)processed2;
  assert(f.transport->callCount() == 1);

  const auto saved2 = f.store->get(id);
  assert(saved2.has_value());
  assert(saved2->status == OperationStatus::PermanentFailed);

  std::cout << "[4] OK: permanent_fail_no_retry\n";
}

static void inflight_timeout_requeue()
{
  SyncEngine::Config cfg;
  cfg.worker_count = 1;
  cfg.batch_limit = 10;
  cfg.idle_sleep_ms = 0;
  cfg.offline_sleep_ms = 0;
  cfg.inflight_timeout_ms = 50;

  auto f = make_fixture("inflight", cfg);

  const auto t0 = now_ms();
  const auto id = f.outbox->enqueue(make_op("http.post", "/api/messages", R"({"text":"inflight test"})"), t0);

  const bool claimed = f.outbox->claim(id, t0);
  assert(claimed);

  {
    const auto saved = f.store->get(id);
    assert(saved.has_value());
    assert(saved->status == OperationStatus::InFlight);
  }

  const auto t1 = t0 + 60;
  f.engine->tick(t1);

  {
    const auto saved = f.store->get(id);
    assert(saved.has_value());
    assert(saved->status != OperationStatus::InFlight);
  }

  f.engine->tick(t1 + 1);

  const auto final = f.store->get(id);
  assert(final.has_value());
  assert(final->status == OperationStatus::Done);

  std::cout << "[5] OK: inflight_timeout_requeue\n";
}

static int run_all()
{
  smoke_success();
  offline_no_send();
  retryable_then_success();
  permanent_fail_no_retry();
  inflight_timeout_requeue();

  std::cout << "\n[sync] ALL OK\n";
  return 0;
}

int main()
{
  return run_all();
}
