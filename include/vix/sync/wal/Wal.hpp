/**
 *
 *  @file Wal.hpp
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
#ifndef VIX_SYNC_WAL_HPP
#define VIX_SYNC_WAL_HPP

#include <cstdint>
#include <filesystem>
#include <functional>

#include <vix/sync/wal/WalRecord.hpp>

namespace vix::sync::wal
{

  class Wal
  {
  public:
    struct Config
    {
      std::filesystem::path file_path{"./.vix/wal.log"};
      bool fsync_on_write{false};
    };

    explicit Wal(Config cfg);
    std::int64_t append(const WalRecord &rec);
    std::int64_t replay(
        std::int64_t from_offset,
        const std::function<void(const WalRecord &)> &on_record);

  private:
    Config cfg_;
  };

} // namespace vix::sync::wal

#endif
