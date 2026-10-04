/**
 *
 *  @file sync_engine_outbox_smoke_test.cpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.  All rights reserved.
 *  https://github.com/vixcpp/vix
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 *
 */
#include <cassert>
#include <filesystem>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/engine/SyncEngine.hpp>

#include "fake_http_transport.hpp"

static void reset_test_dir(const std::filesystem::path &dir)
{
  std::error_code ec;
  std::filesystem::remove_all(dir, ec);
  std::filesystem::create_directories(dir, ec);
}

static std::shared_ptr<vix::sync::outbox::Outbox> make_outbox(
    const std::filesystem::path &dir,
    const char *owner)
{
  using namespace vix::sync::outbox;

  reset_test_dir(dir);
  auto store = std::make_shared<FileOutboxStore>(FileOutboxStore::Config{
      .file_path = dir / "outbox.json",
      .pretty_json = true,
      .fsync_on_write = false});

  return std::make_shared<Outbox>(
      Outbox::Config{.owner = owner},
      std::move(store));
}

static vix::sync::Operation ready_operation()
{
  vix::sync::Operation op;
  op.kind = "http.post";
  op.target = "/api/messages";
  op.payload = R"({"text":"hello offline"})";
  return op;
}

static void test_online_send_and_completion()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  auto outbox = make_outbox("./.vix_test", "test-engine");
  auto transport = std::make_shared<FakeHttpTransport>();
  transport->setDefault({.ok = true});
  SyncEngine::Config config{.worker_count = 1, .batch_limit = 10};
  config.send_permission = [] { return true; };

  SyncEngine engine(
      std::move(config), outbox, transport);

  constexpr std::int64_t t0 = 1'000;
  const auto id = outbox->enqueue(ready_operation(), t0);
  assert(engine.tick(t0) >= 1);

  const auto saved = outbox->store()->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);
}

static void test_offline_permission_blocks_ready_send()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  constexpr std::int64_t t0 = 10'000;
  auto outbox = make_outbox("./.vix_test_offline_gate", "offline-gate");
  int probe_calls = 0;
  auto transport = std::make_shared<FakeHttpTransport>();
  const auto id = outbox->enqueue(ready_operation(), t0);
  SyncEngine::Config config;
  config.send_permission = [&probe_calls]
  {
    ++probe_calls;
    return false;
  };

  SyncEngine engine(std::move(config), outbox, transport);

  assert(engine.tick(t0) == 0);
  assert(probe_calls == 1);
  assert(transport->callCount() == 0);

  const auto saved = outbox->store()->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Pending);
}

static void test_workers_share_rate_limited_permission_state()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  constexpr std::int64_t t0 = 20'000;
  auto outbox = make_outbox("./.vix_test_shared_gate", "shared-gate");
  int probe_calls = 0;
  auto transport = std::make_shared<FakeHttpTransport>();
  outbox->enqueue(ready_operation(), t0);
  SyncEngine::Config config{.worker_count = 2};
  config.send_permission_min_interval_ms = 100;
  config.send_permission = [&probe_calls]
  {
    ++probe_calls;
    return false;
  };

  SyncEngine engine(
      std::move(config), outbox, transport);

  assert(engine.tick(t0) == 0);
  assert(probe_calls == 1);
  assert(transport->callCount() == 0);
}

static void test_offline_to_online_recovery_after_rate_limit()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  constexpr std::int64_t t0 = 30'000;
  auto outbox = make_outbox("./.vix_test_gate_recovery", "gate-recovery");
  int probe_calls = 0;
  auto transport = std::make_shared<FakeHttpTransport>();
  const auto id = outbox->enqueue(ready_operation(), t0);
  SyncEngine::Config config;
  config.send_permission_min_interval_ms = 100;
  config.send_permission = [&probe_calls]
  {
    ++probe_calls;
    return probe_calls >= 2;
  };

  SyncEngine engine(std::move(config), outbox, transport);

  assert(engine.tick(t0) == 0);
  assert(probe_calls == 1);
  assert(transport->callCount() == 0);

  assert(engine.tick(t0 + 50) == 0);
  assert(probe_calls == 1);
  assert(transport->callCount() == 0);

  assert(engine.tick(t0 + 100) >= 1);
  assert(probe_calls == 2);
  assert(transport->callCount() == 1);

  const auto saved = outbox->store()->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);
}

static void test_probe_exception_propagates_from_manual_tick()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  auto outbox = make_outbox("./.vix_test_gate_exception", "gate-exception");
  auto transport = std::make_shared<FakeHttpTransport>();
  outbox->enqueue(ready_operation(), 40'000);
  SyncEngine::Config config;
  config.send_permission = []
  {
    throw std::runtime_error("permission failure");
    return false;
  };

  SyncEngine engine(std::move(config), outbox, transport);

  bool propagated = false;
  try
  {
    (void)engine.tick(40'000);
  }
  catch (const std::runtime_error &)
  {
    propagated = true;
  }

  assert(propagated);
  assert(transport->callCount() == 0);
}

static void test_null_probe_allows_send()
{
  using namespace vix::sync;
  using namespace vix::sync::engine;

  constexpr std::int64_t t0 = 50'000;
  auto outbox = make_outbox("./.vix_test_null_gate", "null-gate");
  auto transport = std::make_shared<FakeHttpTransport>();
  const auto id = outbox->enqueue(ready_operation(), t0);

  SyncEngine engine(
      SyncEngine::Config{},
      outbox,
      transport);

  assert(engine.tick(t0) >= 1);
  assert(transport->callCount() == 1);

  const auto saved = outbox->store()->get(id);
  assert(saved.has_value());
  assert(saved->status == OperationStatus::Done);
}

int main()
{
  test_online_send_and_completion();
  test_offline_permission_blocks_ready_send();
  test_workers_share_rate_limited_permission_state();
  test_offline_to_online_recovery_after_rate_limit();
  test_probe_exception_propagates_from_manual_tick();
  test_null_probe_allows_send();

  std::cout << "OK: sync send-gating regression tests passed\n";
  return 0;
}
