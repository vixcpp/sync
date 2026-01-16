/**
 *
 *  @file SyncWorker.hpp
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
#ifndef VIX_SYNC_WORKER_HPP
#define VIX_SYNC_WORKER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <vix/sync/Operation.hpp>
#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/outbox/Outbox.hpp>

namespace vix::sync::engine
{

  struct SendResult
  {
    bool ok{false};
    bool retryable{true};
    std::string error;
  };

  class ISyncTransport
  {
  public:
    virtual ~ISyncTransport() = default;
    virtual SendResult send(const vix::sync::Operation &op) = 0;
  };

  class SyncWorker
  {
  public:
    struct Config
    {
      std::size_t batch_limit{25};
      std::int64_t idle_sleep_ms{250};
      std::int64_t offline_sleep_ms{500};
      std::int64_t inflight_timeout_ms{10'000};
    };

    SyncWorker(
        Config cfg,
        std::shared_ptr<vix::sync::outbox::Outbox> outbox,
        std::shared_ptr<vix::net::NetworkProbe> probe,
        std::shared_ptr<ISyncTransport> transport);
    std::size_t tick(std::int64_t now_ms);

  private:
    bool should_send_(std::int64_t now_ms);
    std::size_t process_ready_(std::int64_t now_ms);

  private:
    Config cfg_;
    std::shared_ptr<vix::sync::outbox::Outbox> outbox_;
    std::shared_ptr<vix::net::NetworkProbe> probe_;
    std::shared_ptr<ISyncTransport> transport_;
  };

} // namespace vix::sync::engine

#endif
