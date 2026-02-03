/**
 *
 *  @file FileOutboxStore.hpp
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
#ifndef VIX_FILE_OUTBOX_STORE_HPP
#define VIX_FILE_OUTBOX_STORE_HPP

#include <cstdint>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include <vix/sync/Operation.hpp>
#include <vix/sync/outbox/OutboxStore.hpp>

namespace vix::sync::outbox
{
  /**
   * @brief File-backed implementation of the OutboxStore interface.
   *
   * FileOutboxStore persists all outbox operations into a single JSON file.
   * It is designed as a simple, durable default store suitable for:
   * - offline-first environments
   * - crash recovery and restart safety
   * - local-first operation without external dependencies
   *
   * The store lazily loads data on first access and keeps an in-memory
   * representation protected by a mutex. Mutations are flushed back to disk
   * according to the configured persistence policy.
   *
   * @note This store favors correctness and simplicity over high throughput.
   * For large-scale or high-concurrency scenarios, a database-backed store
   * may be more appropriate.
   */
  class FileOutboxStore final : public OutboxStore
  {
  public:
    /**
     * @brief Configuration for FileOutboxStore.
     */
    struct Config
    {
      /**
       * @brief Path to the JSON file used for persistence.
       */
      std::filesystem::path file_path{"./.vix/outbox.json"};

      /**
       * @brief Whether to pretty-print the JSON output.
       *
       * Useful for debugging and inspection, but increases file size.
       */
      bool pretty_json{false};

      /**
       * @brief Whether to fsync() the file after each write.
       *
       * When enabled, provides stronger durability guarantees at the
       * cost of performance.
       */
      bool fsync_on_write{false};
    };

    /**
     * @brief Construct a file-based outbox store.
     *
     * The underlying file is not loaded immediately; loading happens lazily
     * on first access.
     *
     * @param cfg Store configuration.
     */
    explicit FileOutboxStore(Config cfg);

    /**
     * @brief Insert or update an operation in the outbox.
     *
     * @param op Operation to persist.
     */
    void put(const vix::sync::Operation &op) override;

    /**
     * @brief Retrieve an operation by its identifier.
     *
     * @param id Operation identifier.
     * @return Optional operation if found.
     */
    std::optional<vix::sync::Operation> get(const std::string &id) override;

    /**
     * @brief List operations matching the given options.
     *
     * @param opt Filtering and ordering options.
     * @return Vector of matching operations.
     */
    std::vector<vix::sync::Operation> list(const ListOptions &opt) override;

    /**
     * @brief Claim an operation for processing.
     *
     * Marks the operation as in-flight and associates it with an owner.
     *
     * @param id Operation identifier.
     * @param owner Worker or engine identifier.
     * @param now_ms Current time in milliseconds.
     * @return true if the operation was successfully claimed.
     */
    bool claim(
        const std::string &id,
        const std::string &owner,
        std::int64_t now_ms) override;

    /**
     * @brief Mark an operation as successfully completed.
     *
     * @param id Operation identifier.
     * @param now_ms Completion time in milliseconds.
     * @return true if the operation was found and updated.
     */
    bool mark_done(const std::string &id, std::int64_t now_ms) override;

    /**
     * @brief Mark an operation as failed with a retry scheduled.
     *
     * @param id Operation identifier.
     * @param error Error message for diagnostics.
     * @param now_ms Failure time in milliseconds.
     * @param next_retry_at_ms Time when the operation becomes eligible for retry.
     * @return true if the operation was found and updated.
     */
    bool mark_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms,
        std::int64_t next_retry_at_ms) override;

    /**
     * @brief Remove completed operations older than a given threshold.
     *
     * @param older_than_ms Cutoff time in milliseconds.
     * @return Number of pruned operations.
     */
    std::size_t prune_done(std::int64_t older_than_ms) override;

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
    bool mark_permanent_failed(
        const std::string &id,
        const std::string &error,
        std::int64_t now_ms) override;

    /**
     * @brief Requeue in-flight operations that exceeded a timeout.
     *
     * Operations claimed but not completed within the timeout window
     * are made eligible for retry.
     *
     * @param now_ms Current time in milliseconds.
     * @param timeout_ms In-flight timeout threshold.
     * @return Number of operations re-queued.
     */
    std::size_t requeue_inflight_older_than(
        std::int64_t now_ms,
        std::int64_t timeout_ms) override;

  private:
    /**
     * @brief Load the JSON file into memory if not already loaded.
     *
     * This method is called lazily on first access and is thread-safe.
     */
    void load_if_needed_();

    /**
     * @brief Flush the in-memory state back to disk.
     *
     * Writes the JSON file and applies fsync() if configured.
     */
    void flush_();

  private:
    /**
     * @brief Store configuration.
     */
    Config cfg_;

    /**
     * @brief Mutex protecting all internal state.
     */
    std::mutex mu_;

    /**
     * @brief Whether the store has been loaded from disk.
     */
    bool loaded_{false};

    /**
     * @brief Map of operation id to operation data.
     */
    std::unordered_map<std::string, vix::sync::Operation> ops_;

    /**
     * @brief Map of operation id to current owner (in-flight).
     */
    std::unordered_map<std::string, std::string> owner_;
  };

} // namespace vix::sync::outbox

#endif // VIX_FILE_OUTBOX_STORE_HPP
