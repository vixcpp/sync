/**
 *
 *  @file OutboxStore.hpp
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
#ifndef VIX_OUTBOX_STORE_HPP
#define VIX_OUTBOX_STORE_HPP

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include <vix/sync/Operation.hpp>

namespace vix::sync::outbox
{

  struct ListOptions
  {
    std::size_t limit{50};
    std::int64_t now_ms{0};
    bool only_ready{true};
    bool include_inflight{false};
  };

  class OutboxStore
  {
  public:
    virtual ~OutboxStore() = default;
    virtual void put(const vix::sync::Operation &op) = 0;
    virtual std::optional<vix::sync::Operation> get(const std::string &id) = 0;
    virtual std::vector<vix::sync::Operation> list(const ListOptions &opt) = 0;
    virtual bool claim(const std::string &id, const std::string &owner, std::int64_t now_ms) = 0;
    virtual bool mark_done(const std::string &id, std::int64_t now_ms) = 0;
    virtual bool mark_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms,
        std::int64_t next_retry_at_ms) = 0;

    // Optional cleanup
    virtual std::size_t prune_done(std::int64_t older_than_ms) = 0;
    virtual bool mark_permanent_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms) = 0;

    virtual std::size_t requeue_inflight_older_than(
        std::int64_t now_ms,
        std::int64_t timeout_ms) = 0;
  };

} // namespace vix::sync::outbox

#endif
