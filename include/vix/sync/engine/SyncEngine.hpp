/**
 *
 *  @file SyncEngine.hpp
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
#ifndef VIX_SYNC_ENGINE_HPP
#define VIX_SYNC_ENGINE_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/engine/SyncWorker.hpp>

namespace vix::sync::engine
{

  class SyncEngine
  {
  public:
    struct Config
    {
      std::size_t worker_count{1};
      std::int64_t idle_sleep_ms{250};
      std::int64_t offline_sleep_ms{500};
      std::size_t batch_limit{25};
      std::int64_t inflight_timeout_ms{10'000};
    };

    SyncEngine(
        Config cfg,
        std::shared_ptr<vix::sync::outbox::Outbox> outbox,
        std::shared_ptr<vix::net::NetworkProbe> probe,
        std::shared_ptr<ISyncTransport> transport);

    ~SyncEngine();
    std::size_t tick(std::int64_t now_ms);
    void start();
    void stop();
    bool running() const noexcept { return running_.load(); }

  private:
    void run_loop_();

  private:
    Config cfg_;
    std::shared_ptr<vix::sync::outbox::Outbox> outbox_;
    std::shared_ptr<vix::net::NetworkProbe> probe_;
    std::shared_ptr<ISyncTransport> transport_;
    std::vector<std::unique_ptr<SyncWorker>> workers_;
    std::atomic<bool> running_{false};
    std::thread thread_;
  };

} // namespace vix::sync::engine

#endif
