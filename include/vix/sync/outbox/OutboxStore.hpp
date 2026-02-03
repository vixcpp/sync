/**
 *
 *  @file OutboxStore.hpp
 *  @author Gaspard Kirira
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
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
  /**
   * @brief Options controlling listing behavior for outbox operations.
   *
   * ListOptions are passed to OutboxStore::list() to filter and limit
   * the returned operations based on state and timing.
   */
  struct ListOptions
  {
    /**
     * @brief Maximum number of operations to return.
     */
    std::size_t limit{50};

    /**
     * @brief Current time in milliseconds.
     *
     * Used to determine readiness (retry timing, scheduling).
     */
    std::int64_t now_ms{0};

    /**
     * @brief Only return operations that are ready to be processed.
     *
     * When true, operations that are waiting for retry are excluded.
     */
    bool only_ready{true};

    /**
     * @brief Include operations currently marked as in-flight.
     */
    bool include_inflight{false};
  };

  /**
   * @brief Abstract persistence interface for the durable outbox.
   *
   * OutboxStore defines the minimal contract required by the Outbox:
   * - durable persistence of operations
   * - state transitions (claimed, done, failed)
   * - basic cleanup and recovery helpers
   *
   * Implementations may be file-based, database-backed, or in-memory
   * (for testing), but must preserve the correctness guarantees of
   * the outbox pattern.
   *
   * @note Thread-safety guarantees are implementation-defined. The
   * Outbox assumes that concurrent access is handled appropriately
   * by the store.
   */
  class OutboxStore
  {
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~OutboxStore() = default;

    /**
     * @brief Insert or update an operation in the store.
     *
     * @param op Operation to persist.
     */
    virtual void put(const vix::sync::Operation &op) = 0;

    /**
     * @brief Retrieve an operation by its identifier.
     *
     * @param id Operation identifier.
     * @return Optional operation if found.
     */
    virtual std::optional<vix::sync::Operation> get(const std::string &id) = 0;

    /**
     * @brief List operations matching the given options.
     *
     * @param opt Listing options.
     * @return Vector of operations.
     */
    virtual std::vector<vix::sync::Operation> list(const ListOptions &opt) = 0;

    /**
     * @brief Claim an operation for processing.
     *
     * Marks the operation as in-flight and associates it with an owner.
     *
     * @param id Operation identifier.
     * @param owner Logical owner claiming the operation.
     * @param now_ms Current time in milliseconds.
     * @return true if the operation was successfully claimed.
     */
    virtual bool claim(
        const std::string &id,
        const std::string &owner,
        std::int64_t now_ms) = 0;

    /**
     * @brief Mark an operation as successfully completed.
     *
     * @param id Operation identifier.
     * @param now_ms Completion time in milliseconds.
     * @return true if the operation was found and updated.
     */
    virtual bool mark_done(
        const std::string &id,
        std::int64_t now_ms) = 0;

    /**
     * @brief Mark an operation as failed with a scheduled retry.
     *
     * @param id Operation identifier.
     * @param error Error message for diagnostics.
     * @param now_ms Failure time in milliseconds.
     * @param next_retry_at_ms Time when the operation becomes eligible for retry.
     * @return true if the operation was found and updated.
     */
    virtual bool mark_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms,
        std::int64_t next_retry_at_ms) = 0;

    /**
     * @brief Remove completed operations older than a given threshold.
     *
     * @param older_than_ms Cutoff time in milliseconds.
     * @return Number of pruned operations.
     */
    virtual std::size_t prune_done(std::int64_t older_than_ms) = 0;

    /**
     * @brief Mark an operation as permanently failed.
     *
     * Permanently failed operations are not retried.
     *
     * @param id Operation identifier.
     * @param error Error message for diagnostics.
     * @param now_ms Failure time in milliseconds.
     * @return true if the operation was found and updated.
     */
    virtual bool mark_permanent_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms) = 0;

    /**
     * @brief Requeue in-flight operations that exceeded a timeout.
     *
     * This is typically used for crash recovery or worker failure detection.
     *
     * @param now_ms Current time in milliseconds.
     * @param timeout_ms In-flight timeout threshold.
     * @return Number of operations re-queued.
     */
    virtual std::size_t requeue_inflight_older_than(
        std::int64_t now_ms,
        std::int64_t timeout_ms) = 0;
  };

} // namespace vix::sync::outbox

#endif // VIX_OUTBOX_STORE_HPP
