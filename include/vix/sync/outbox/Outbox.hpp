/**
 *
 *  @file Outbox.hpp
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
#ifndef VIX_OUTBOX_HPP
#define VIX_OUTBOX_HPP

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <vix/sync/Operation.hpp>
#include <vix/sync/RetryPolicy.hpp>
#include <vix/sync/outbox/OutboxStore.hpp>

namespace vix::sync::outbox
{
  /**
   * @brief Durable outbox coordinating retry, idempotency and ownership.
   *
   * Outbox is the high-level facade sitting above an OutboxStore. It implements
   * the core outbox pattern:
   * - All operations are persisted before any network attempt
   * - Operations are claimed before being processed
   * - Completion, retry and permanent failure are recorded durably
   *
   * The Outbox itself contains no I/O logic. It delegates persistence to an
   * OutboxStore and retry decisions to RetryPolicy.
   *
   * @note Thread-safety depends on the underlying OutboxStore implementation.
   * Outbox assumes the store provides the necessary synchronization guarantees.
   */
  class Outbox
  {
  public:
    /**
     * @brief Configuration for the Outbox.
     */
    struct Config
    {
      /**
       * @brief Logical owner name used when claiming operations.
       *
       * Typically identifies the engine or worker instance.
       */
      std::string owner{"vix-sync"};

      /**
       * @brief Retry policy applied to failed operations.
       */
      RetryPolicy retry{};

      /**
       * @brief Automatically generate an operation id if missing.
       */
      bool auto_generate_ids{true};

      /**
       * @brief Automatically generate an idempotency key if missing.
       */
      bool auto_generate_idempotency_key{true};
    };

    /**
     * @brief Construct an Outbox.
     *
     * @param cfg Outbox configuration.
     * @param store Persistent store implementation.
     */
    Outbox(Config cfg, std::shared_ptr<OutboxStore> store);

    /**
     * @brief Enqueue a new operation into the outbox.
     *
     * This persists the operation before it becomes eligible for sending.
     * Missing identifiers may be generated according to configuration.
     *
     * @param op Operation to enqueue.
     * @param now_ms Current time in milliseconds.
     * @return Operation identifier.
     */
    std::string enqueue(vix::sync::Operation op, std::int64_t now_ms);

    /**
     * @brief Inspect operations ready to be processed.
     *
     * This does not claim the operations; it only returns candidates that
     * satisfy retry timing and state conditions.
     *
     * @param now_ms Current time in milliseconds.
     * @param limit Maximum number of operations to return.
     * @return Vector of ready operations.
     */
    std::vector<vix::sync::Operation> peek_ready(
        std::int64_t now_ms,
        std::size_t limit = 50);

    /**
     * @brief Claim an operation for processing.
     *
     * @param id Operation identifier.
     * @param now_ms Current time in milliseconds.
     * @return true if the operation was successfully claimed.
     */
    bool claim(const std::string &id, std::int64_t now_ms);

    /**
     * @brief Mark an operation as successfully completed.
     *
     * @param id Operation identifier.
     * @param now_ms Completion time in milliseconds.
     * @return true if the operation was found and updated.
     */
    bool complete(const std::string &id, std::int64_t now_ms);

    /**
     * @brief Mark an operation as failed.
     *
     * Depending on retryable and RetryPolicy, the operation may be scheduled
     * for retry or marked as permanently failed.
     *
     * @param id Operation identifier.
     * @param error Error message for diagnostics.
     * @param now_ms Failure time in milliseconds.
     * @param retryable Whether the failure is retryable.
     * @return true if the operation was found and updated.
     */
    bool fail(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms,
        bool retryable = true);

    /**
     * @brief Access the underlying store.
     */
    std::shared_ptr<OutboxStore> store() const noexcept { return store_; }

    /**
     * @brief Access the outbox configuration.
     */
    const Config &config() const noexcept { return cfg_; }

  private:
    /**
     * @brief Generate a unique operation identifier.
     */
    static std::string make_id();

    /**
     * @brief Generate an idempotency key.
     */
    static std::string make_idempotency_key();

  private:
    /**
     * @brief Stored configuration.
     */
    Config cfg_;

    /**
     * @brief Persistent store backing the outbox.
     */
    std::shared_ptr<OutboxStore> store_;
  };

} // namespace vix::sync::outbox

#endif // VIX_OUTBOX_HPP
