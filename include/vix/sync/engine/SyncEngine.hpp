/**
 *
 *  @file SyncEngine.hpp
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
#ifndef VIX_SYNC_ENGINE_HPP
#define VIX_SYNC_ENGINE_HPP

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

#include <vix/net/NetworkProbe.hpp>
#include <vix/sync/engine/SyncWorker.hpp>
#include <vix/sync/outbox/Outbox.hpp>

namespace vix::sync::engine
{
  /**
   * @brief Coordinator for the offline-first sync loop.
   *
   * SyncEngine is responsible for running the main synchronization control loop:
   * - Spawns and owns a set of SyncWorker instances
   * - Periodically ticks workers to pull operations from the Outbox
   * - Uses NetworkProbe to adapt behavior when offline/online
   * - Delegates actual I/O to an ISyncTransport implementation
   *
   * The engine can be driven manually via tick() (single-threaded integration),
   * or can run its own background thread via start().
   *
   * @note Thread-safety: start()/stop() manage an internal thread. tick() is
   * intended to be called by the engine thread, or by a single external driver.
   * Avoid calling tick() concurrently with start()/stop().
   */
  class SyncEngine
  {
  public:
    /**
     * @brief Configuration values controlling the sync loop behavior.
     *
     * All time values are expressed in milliseconds.
     */
    struct Config
    {
      /**
       * @brief Number of worker instances to create.
       */
      std::size_t worker_count{1};

      /**
       * @brief Sleep duration when there is nothing to do (engine is idle).
       */
      std::int64_t idle_sleep_ms{250};

      /**
       * @brief Sleep duration when network is considered offline.
       */
      std::int64_t offline_sleep_ms{500};

      /**
       * @brief Maximum number of operations to pull per batch.
       */
      std::size_t batch_limit{25};

      /**
       * @brief Maximum time an operation is allowed to remain in-flight.
       *
       * In-flight operations older than this may be re-queued or marked
       * for retry depending on worker logic.
       */
      std::int64_t inflight_timeout_ms{10'000};
    };

    /**
     * @brief Construct a SyncEngine with its dependencies.
     *
     * @param cfg Engine configuration.
     * @param outbox Shared Outbox containing pending operations to sync.
     * @param probe Network probe used to detect offline/online state.
     * @param transport Transport used to send/receive sync payloads.
     */
    SyncEngine(
        Config cfg,
        std::shared_ptr<vix::sync::outbox::Outbox> outbox,
        std::shared_ptr<vix::net::NetworkProbe> probe,
        std::shared_ptr<ISyncTransport> transport);

    /**
     * @brief Destroy the engine.
     *
     * Ensures the engine is stopped and joins the internal thread if running.
     */
    ~SyncEngine();

    /**
     * @brief Execute one engine iteration.
     *
     * This performs a single "tick" of the engine and its workers using the
     * provided current time (milliseconds).
     *
     * Typical use cases:
     * - Manual driving (integration into an existing loop)
     * - Unit testing deterministic behavior by controlling time
     *
     * @param now_ms Current monotonic time in milliseconds.
     * @return std::size_t Number of operations processed (best-effort metric).
     */
    std::size_t tick(std::int64_t now_ms);

    /**
     * @brief Start the internal background loop.
     *
     * If already running, this call has no effect.
     */
    void start();

    /**
     * @brief Request shutdown and stop the background loop.
     *
     * If not running, this call has no effect.
     */
    void stop();

    /**
     * @brief Check whether the engine background loop is running.
     *
     * @return true if running, false otherwise.
     */
    bool running() const noexcept { return running_.load(); }

  private:
    /**
     * @brief Internal background thread loop.
     *
     * Repeatedly calls tick(), then sleeps depending on idle/offline state.
     */
    void run_loop_();

  private:
    /**
     * @brief Stored engine configuration.
     */
    Config cfg_;

    /**
     * @brief Outbox providing operations to be synchronized.
     */
    std::shared_ptr<vix::sync::outbox::Outbox> outbox_;

    /**
     * @brief Network probe used to detect connectivity changes.
     */
    std::shared_ptr<vix::net::NetworkProbe> probe_;

    /**
     * @brief Transport used by workers to communicate with remote peers/edge.
     */
    std::shared_ptr<ISyncTransport> transport_;

    /**
     * @brief Owned workers responsible for processing outbox batches.
     */
    std::vector<std::unique_ptr<SyncWorker>> workers_;

    /**
     * @brief Running flag for the background loop.
     */
    std::atomic<bool> running_{false};

    /**
     * @brief Background thread running run_loop_().
     */
    std::thread thread_;
  };

} // namespace vix::sync::engine

#endif // VIX_SYNC_ENGINE_HPP
