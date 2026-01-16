/**
 *
 *  @file WalRecord.hpp
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
#ifndef VIX_WAL_RECORD_HPP
#define VIX_WAL_RECORD_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace vix::sync::wal
{

  enum class RecordType : std::uint8_t
  {
    PutOperation = 1,
    MarkDone = 2,
    MarkFailed = 3,
  };

  struct WalRecord
  {
    std::string id;
    RecordType type{RecordType::PutOperation};
    std::int64_t ts_ms{0};
    std::vector<std::uint8_t> payload;
    std::string error;
    std::int64_t next_retry_at_ms{0};
  };

} // namespace vix::sync::wal

#endif
