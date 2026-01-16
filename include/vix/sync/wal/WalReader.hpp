/**
 *
 *  @file WalReader.hpp
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
#ifndef VIX_WAL_READER_HPP
#define VIX_WAL_READER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <optional>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{

  class WalReader
  {
  public:
    explicit WalReader(std::filesystem::path file_path);
    void seek(std::int64_t offset);
    std::optional<WalRecord> next();
    std::int64_t current_offset() const noexcept { return offset_; }

  private:
    void open_();
    std::filesystem::path file_path_;
    std::ifstream in_;
    std::int64_t offset_{0};
  };

} // namespace vix::sync::wal

#endif
