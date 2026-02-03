/**
 *
 *  @file WalReader.hpp
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
#ifndef VIX_WAL_READER_HPP
#define VIX_WAL_READER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{
  /**
   * @brief Sequential reader for a write-ahead log (WAL).
   *
   * WalReader provides a simple forward-only interface to iterate over
   * WalRecord entries stored in a WAL file.
   *
   * It is typically used during recovery or replay to reconstruct state
   * by reading records in the exact order they were written.
   */
  class WalReader
  {
  public:
    /**
     * @brief Construct a WAL reader for the given file.
     *
     * The file is not opened until the first read or seek operation.
     *
     * @param file_path Path to the WAL file.
     */
    explicit WalReader(std::filesystem::path file_path);

    /**
     * @brief Seek to a specific offset in the WAL.
     *
     * Subsequent calls to next() will return records starting
     * from this offset.
     *
     * @param offset Byte offset within the WAL file.
     */
    void seek(std::int64_t offset);

    /**
     * @brief Read the next record from the WAL.
     *
     * @return Optional WalRecord if available, or std::nullopt on EOF.
     */
    std::optional<WalRecord> next();

    /**
     * @brief Get the current read offset.
     */
    std::int64_t current_offset() const noexcept { return offset_; }

  private:
    /**
     * @brief Open the WAL file if not already open.
     */
    void open_();

  private:
    /**
     * @brief Path to the WAL file.
     */
    std::filesystem::path file_path_;

    /**
     * @brief Input file stream used for reading.
     */
    std::ifstream in_;

    /**
     * @brief Current read offset in bytes.
     */
    std::int64_t offset_{0};
  };

} // namespace vix::sync::wal

#endif // VIX_WAL_READER_HPP
