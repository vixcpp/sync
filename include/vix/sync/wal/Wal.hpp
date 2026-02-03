/**
 *
 *  @file Wal.hpp
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
#ifndef VIX_SYNC_WAL_HPP
#define VIX_SYNC_WAL_HPP

#include <cstdint>
#include <filesystem>
#include <functional>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{
  /**
   * @brief Write-Ahead Log (WAL) for durable sync operations.
   *
   * Wal implements a minimal append-only write-ahead log used to persist
   * operations before they are applied or synchronized.
   *
   * Core guarantees:
   * - Records are appended sequentially to a file
   * - Each append returns a monotonically increasing offset
   * - On restart, the log can be replayed from any offset to restore state
   *
   * This component is a foundational building block for offline-first and
   * crash-safe synchronization.
   */
  class Wal
  {
  public:
    /**
     * @brief Configuration for the WAL.
     */
    struct Config
    {
      /**
       * @brief Path to the WAL file.
       */
      std::filesystem::path file_path{"./.vix/wal.log"};

      /**
       * @brief Whether to fsync() the file after each append.
       *
       * When enabled, provides stronger durability guarantees at the cost
       * of write performance.
       */
      bool fsync_on_write{false};
    };

    /**
     * @brief Construct a WAL with the given configuration.
     *
     * @param cfg WAL configuration.
     */
    explicit Wal(Config cfg);

    /**
     * @brief Append a record to the log.
     *
     * The record is serialized and written at the end of the file.
     *
     * @param rec Record to append.
     * @return Offset of the appended record.
     */
    std::int64_t append(const WalRecord &rec);

    /**
     * @brief Replay records starting from a given offset.
     *
     * Iterates over all records from the specified offset and invokes
     * the provided callback for each record in order.
     *
     * @param from_offset Offset to start replaying from.
     * @param on_record Callback invoked for each record.
     * @return Offset of the last replayed record.
     */
    std::int64_t replay(
        std::int64_t from_offset,
        const std::function<void(const WalRecord &)> &on_record);

  private:
    /**
     * @brief Stored WAL configuration.
     */
    Config cfg_;
  };

} // namespace vix::sync::wal

#endif // VIX_SYNC_WAL_HPP
