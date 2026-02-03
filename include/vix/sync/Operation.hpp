/**
 *
 *  @file Operation.hpp
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
#ifndef VIX_SYNC_OPERATION_HPP
#define VIX_SYNC_OPERATION_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace vix::sync
{
  /**
   * @brief Lifecycle status of a sync operation.
   *
   * OperationStatus represents the durable state of an operation
   * as it flows through the sync pipeline.
   */
  enum class OperationStatus : std::uint8_t
  {
    /**
     * @brief Operation is persisted and waiting to be processed.
     */
    Pending = 0,

    /**
     * @brief Operation has been claimed and is currently being processed.
     */
    InFlight,

    /**
     * @brief Operation completed successfully.
     */
    Done,

    /**
     * @brief Operation failed but may be retried.
     */
    Failed,

    /**
     * @brief Operation failed permanently and will not be retried.
     */
    PermanentFailed
  };

  /**
   * @brief Durable representation of a single sync operation.
   *
   * Operation is the core unit stored in the Outbox and WAL. It is designed
   * to be:
   * - immutable in intent (what to do)
   * - mutable in state (status, attempts, timestamps)
   *
   * The operation lifecycle is fully driven by durable state transitions,
   * enabling crash-safe retries and deterministic recovery.
   */
  struct Operation
  {
    /**
     * @brief Unique identifier of the operation.
     */
    std::string id;

    /**
     * @brief Logical kind or type of operation.
     *
     * Examples: "http.request", "p2p.message", "db.mutation".
     */
    std::string kind;

    /**
     * @brief Target of the operation.
     *
     * Examples: URL, peer id, resource name.
     */
    std::string target;

    /**
     * @brief Opaque payload associated with the operation.
     */
    std::string payload;

    /**
     * @brief Idempotency key used to deduplicate retries.
     */
    std::string idempotency_key;

    /**
     * @brief Creation timestamp in milliseconds.
     */
    std::int64_t created_at_ms{0};

    /**
     * @brief Last update timestamp in milliseconds.
     */
    std::int64_t updated_at_ms{0};

    /**
     * @brief Number of delivery attempts so far.
     */
    std::uint32_t attempt{0};

    /**
     * @brief Next time when the operation becomes eligible for retry.
     */
    std::int64_t next_retry_at_ms{0};

    /**
     * @brief Current lifecycle status.
     */
    OperationStatus status{OperationStatus::Pending};

    /**
     * @brief Last error message, if any.
     */
    std::string last_error;

    /**
     * @brief Check whether the operation is completed.
     */
    bool is_done() const noexcept { return status == OperationStatus::Done; }

    /**
     * @brief Check whether the operation is pending.
     */
    bool is_pending() const noexcept { return status == OperationStatus::Pending; }

    /**
     * @brief Check whether the operation is failed (retryable).
     */
    bool is_failed() const noexcept { return status == OperationStatus::Failed; }

    /**
     * @brief Mark the operation as failed.
     *
     * @param err Error message.
     * @param now_ms Current time in milliseconds.
     */
    void fail(std::string err, std::int64_t now_ms)
    {
      last_error = std::move(err);
      status = OperationStatus::Failed;
      updated_at_ms = now_ms;
    }

    /**
     * @brief Mark the operation as successfully completed.
     *
     * @param now_ms Completion time in milliseconds.
     */
    void done(std::int64_t now_ms)
    {
      status = OperationStatus::Done;
      updated_at_ms = now_ms;
      last_error.clear();
    }
  };

} // namespace vix::sync

#endif // VIX_SYNC_OPERATION_HPP
