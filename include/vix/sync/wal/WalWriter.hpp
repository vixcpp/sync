/**
 *
 *  @file WalWriter.hpp
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
#ifndef VIX_WAL_WRITER_HPP
#define VIX_WAL_WRITER_HPP

#include <cstdint>
#include <filesystem>
#include <fstream>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{

  class WalWriter
  {
  public:
    struct Config
    {
      std::filesystem::path file_path;
      bool fsync_on_write{false};
    };

    explicit WalWriter(Config cfg);
    ~WalWriter();
    std::int64_t append(const WalRecord &rec);
    void flush();

  private:
    void open_();
    std::int64_t tell_();
    Config cfg_;
    std::ofstream out_;
  };

} // namespace vix::sync::wal

#endif
