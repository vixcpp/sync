/**
 *
 *  @file SyncWorker.hpp
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
#ifndef VIX_SYNC_WORKER_HPP
#define VIX_SYNC_WORKER_HPP

#include <cstdint>
#include <functional>
#include <memory>
#include <string>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/Operation.hpp>
#include <vix/sync/outbox/Outbox.hpp>

namespace vix::sync::engine
{
  /**
   * @brief Result of attempting to send a single sync operation.
   *
   * A transport returns SendResult to indicate whether the operation was
   * successfully delivered, and whether failures are retryable.
   */
  struct SendResult
  {
    /**
     * @brief True if the operation was successfully sent/accepted.
     */
    bool ok{false};

    /**
     * @brief True if the failure can be retried safely.
     *
     * When false, the worker may decide to drop or dead-letter the operation
     * (depending on the Outbox policy) instead of retrying indefinitely.
     */
    bool retryable{true};

    /**
     * @brief Optional error message for diagnostics/logging.
     */
    std::string error;
  };

  /**
   * @brief Abstract transport used by the sync worker to deliver operations.
   *
   * Implementations can target HTTP, WebSocket, P2P, edge store-and-forward,
   * or any other delivery mechanism.
   *
   * The worker is intentionally transport-agnostic: it pushes operations and
   * reacts to the SendResult contract.
   */
  class ISyncTransport
  {
  public:
    /**
     * @brief Virtual destructor.
     */
    virtual ~ISyncTransport() = default;

    /**
     * @brief Send one operation to the remote side.
     *
     * @param op Operation to deliver.
     * @return SendResult describing success/failure and retryability.
     */
    virtual SendResult send(const vix::sync::Operation &op) = 0;
  };

  /**
   * @brief Single-worker unit that processes ready operations from the Outbox.
   *
   * SyncWorker:
   * - Consults NetworkProbe to decide if sending should proceed
   * - Pulls a batch of ready operations from the Outbox
   * - Sends operations through ISyncTransport
   * - Applies retry/backoff decisions by updating Outbox state (implementation-defined)
   *
   * Workers are typically owned and orchestrated by SyncEngine.
   *
   * @note This header defines the coordination API and invariants, while the
   * concrete Outbox behavior (reserve/ack/nack/requeue) is handled by Outbox.
   */
  class SyncWorker
  {
  public:
    /**
     * @brief Configuration values controlling the worker behavior.
     *
     * All time values are expressed in milliseconds.
     */
    struct Config
    {
      /**
       * @brief Maximum number of operations to process per tick.
       */
      std::size_t batch_limit{25};

      /**
       * @brief Sleep duration when there is nothing to process.
       *
       * This is mainly used by higher-level orchestrators (SyncEngine).
       */
      std::int64_t idle_sleep_ms{250};

      /**
       * @brief Sleep duration when network is considered offline.
       *
       * This is mainly used by higher-level orchestrators (SyncEngine).
       */
      std::int64_t offline_sleep_ms{500};

      /**
       * @brief Maximum time an operation is allowed to remain in-flight.
       *
       * If an operation exceeds this window, it may be considered timed out
       * and eligible for retry according to outbox policy.
       */
      std::int64_t inflight_timeout_ms{10'000};
    };

    /**
     * @brief Construct a worker with its dependencies.
     *
     * @param cfg Worker configuration.
     * @param outbox Shared outbox used to fetch and update operations.
     * @param probe Network probe used to detect offline/online state.
     * @param transport Transport used to deliver operations.
     */
    SyncWorker(
        Config cfg,
        std::shared_ptr<vix::sync::outbox::Outbox> outbox,
        std::shared_ptr<vix::net::NetworkProbe> probe,
        std::shared_ptr<ISyncTransport> transport);

    /**
     * @brief Process a batch of operations for the current time.
     *
     * tick() is expected to be called periodically by SyncEngine (or a manual
     * driver). It performs best-effort processing up to batch_limit items.
     *
     * @param now_ms Current monotonic time in milliseconds.
     * @return std::size_t Number of operations processed (best-effort metric).
     */
    std::size_t tick(std::int64_t now_ms);

  private:
    /**
     * @brief Decide whether the worker should attempt sending right now.
     *
     * Typically checks connectivity (NetworkProbe) and possibly Outbox state.
     *
     * @param now_ms Current monotonic time in milliseconds.
     * @return true if sending should proceed, false otherwise.
     */
    bool should_send_(std::int64_t now_ms);

    /**
     * @brief Process operations that are ready to be sent.
     *
     * Pulls a batch from the Outbox and attempts to deliver each operation via
     * the transport. Updates Outbox state according to send outcomes.
     *
     * @param now_ms Current monotonic time in milliseconds.
     * @return std::size_t Number of operations processed.
     */
    std::size_t process_ready_(std::int64_t now_ms);

  private:
    /**
     * @brief Stored worker configuration.
     */
    Config cfg_;

    /**
     * @brief Shared outbox containing operations to send.
     */
    std::shared_ptr<vix::sync::outbox::Outbox> outbox_;

    /**
     * @brief Network probe used to detect connectivity.
     */
    std::shared_ptr<vix::net::NetworkProbe> probe_;

    /**
     * @brief Transport used to send operations.
     */
    std::shared_ptr<ISyncTransport> transport_;
  };

} // namespace vix::sync::engine

#endif // VIX_SYNC_WORKER_HPP
