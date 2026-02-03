/**
 *
 *  @file WalWriter.hpp
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
#ifndef VIX_WAL_WRITER_HPP
#define VIX_WAL_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{
  /**
   * @brief Append-only writer for the write-ahead log (WAL).
   *
   * WalWriter is responsible for serializing and appending WalRecord
   * entries to a WAL file. It provides durable, ordered writes and
   * exposes offsets that can later be used for replay.
   *
   * This class is typically paired with WalReader during recovery.
   */
  class WalWriter
  {
  public:
    /**
     * @brief Configuration for the WAL writer.
     */
    struct Config
    {
      /**
       * @brief Path to the WAL file.
       */
      std::filesystem::path file_path;

      /**
       * @brief Whether to fsync() the file after each append.
       *
       * When enabled, provides stronger durability guarantees at the
       * cost of write performance.
       */
      bool fsync_on_write{false};
    };

    /**
     * @brief Construct a WAL writer.
     *
     * The file is opened lazily on first append.
     *
     * @param cfg Writer configuration.
     */
    explicit WalWriter(Config cfg);

    /**
     * @brief Destroy the WAL writer.
     *
     * Ensures buffered data is flushed before closing the file.
     */
    ~WalWriter();

    /**
     * @brief Append a record to the WAL.
     *
     * The record is serialized and written at the end of the file.
     *
     * @param rec Record to append.
     * @return Offset of the appended record.
     */
    std::int64_t append(const WalRecord &rec);

    /**
     * @brief Flush buffered data to disk.
     *
     * If fsync_on_write is enabled, this may also force data to be
     * persisted to stable storage.
     */
    void flush();

  private:
    /**
     * @brief Open the WAL file if not already open.
     */
    void open_();

    /**
     * @brief Get the current write offset.
     */
    std::int64_t tell_();

  private:
    /**
     * @brief Stored writer configuration.
     */
    Config cfg_;

    /**
     * @brief Output file stream used for writing.
     */
    std::ofstream out_;
  };

} // namespace vix::sync::wal

#endif // VIX_WAL_WRITER_HPP
