/**
 *
 *  @file sync/sync.hpp
 *  @author Gaspard Kirira
 *
 *  @brief Internal aggregation header for the Vix sync module.
 *
 *  This file includes the core synchronization components of Vix,
 *  including operations, retry policies, the sync engine, outbox
 *  storage, and WAL primitives.
 *
 *  For most use cases, prefer:
 *    #include <vix/sync.hpp>
 *
 *  Copyright 2025, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/vix
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the License file.
 *
 *  Vix.cpp
 */
#ifndef VIX_SYNC_SYNC_HPP
#define VIX_SYNC_SYNC_HPP

#include <vix/sync/Operation.hpp>
#include <vix/sync/RetryPolicy.hpp>

// engine
#include <vix/sync/engine/SyncEngine.hpp>
#include <vix/sync/engine/SyncWorker.hpp>

// outbox
#include <vix/sync/outbox/FileOutboxStore.hpp>
#include <vix/sync/outbox/Outbox.hpp>
#include <vix/sync/outbox/OutboxStore.hpp>

// wal
#include <vix/sync/wal/Wal.hpp>
#include <vix/sync/wal/WalReader.hpp>
#include <vix/sync/wal/WalRecord.hpp>
#include <vix/sync/wal/WalWriter.hpp>

#endif // VIX_SYNC_SYNC_HPP
