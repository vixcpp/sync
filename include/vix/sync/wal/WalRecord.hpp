/**
 *
 *  @file WalRecord.hpp
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
#ifndef VIX_WAL_RECORD_HPP
#define VIX_WAL_RECORD_HPP

#include <cstdint>
#include <string>
#include <vector>

namespace vix::sync::wal
{
  /**
   * @brief Type of record stored in the write-ahead log.
   *
   * Each record represents a durable state transition or intent
   * in the sync pipeline.
   */
  enum class RecordType : std::uint8_t
  {
    /**
     * @brief A new operation was added to the system.
     *
     * Payload typically contains a serialized Operation.
     */
    PutOperation = 1,

    /**
     * @brief An operation was successfully completed.
     */
    MarkDone = 2,

    /**
     * @brief An operation failed and may be retried.
     *
     * Additional error and retry metadata may be present.
     */
    MarkFailed = 3,
  };

  /**
   * @brief Single record entry in the write-ahead log (WAL).
   *
   * WalRecord is an append-only, immutable description of a state change.
   * Records are written before any external side effect occurs, ensuring:
   * - durability across crashes
   * - deterministic replay
   * - correct recovery of in-flight and pending operations
   *
   * During replay, records are processed in order to reconstruct
   * Outbox and sync engine state.
   */
  struct WalRecord
  {
    /**
     * @brief Identifier of the affected operation.
     */
    std::string id;

    /**
     * @brief Type of this WAL record.
     */
    RecordType type{RecordType::PutOperation};

    /**
     * @brief Timestamp when the record was created (milliseconds).
     */
    std::int64_t ts_ms{0};

    /**
     * @brief Opaque payload associated with the record.
     *
     * For PutOperation, this usually contains a serialized Operation.
     * For other record types, this may be empty.
     */
    std::vector<std::uint8_t> payload;

    /**
     * @brief Optional error message (used for failure records).
     */
    std::string error;

    /**
     * @brief Next retry timestamp in milliseconds.
     *
     * Used when type == MarkFailed and the failure is retryable.
     */
    std::int64_t next_retry_at_ms{0};
  };

} // namespace vix::sync::wal

#endif // VIX_WAL_RECORD_HPP
