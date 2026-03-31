# Vix Sync

**Durable. Offline-first. Deterministic.**

Vix Sync is the core synchronization engine of Vix.cpp.

It guarantees that every operation:
- is persisted before being sent
- is never lost
- is retried deterministically
- converges to a consistent state

---

## Why Vix Sync exists

Real-world systems are not reliable:
- network drops
- servers restart
- requests fail halfway

Traditional systems:
- lose data
- duplicate requests
- become inconsistent

Vix Sync solves this with a simple rule:

> Write locally. Persist first. Sync later.

---

## Core principles

1. **Durability first**
   - Every operation is stored before any network call

2. **Deterministic retries**
   - Retry logic is reproducible and replayable

3. **Offline-first**
   - The system works without network

4. **Safe convergence**
   - Eventually, all nodes reach a consistent state

---

## Architecture

Vix Sync is composed of small deterministic building blocks:

```text
Local Write
↓
WAL (Write-Ahead Log)
↓
Outbox (durable queue)
↓
SyncWorker
↓
Transport (HTTP / P2P / custom)
↓
Done / Retry / Failed
```

---

## Components

### 1. Operation

A durable unit of work.

```cpp
struct Operation {
  std::string id;
  std::string kind;
  std::string target;
  std::string payload;
};
```

### 2. WAL (Write-Ahead Log)

Append-only log:

- crash-safe
- replayable
- ordered

```cpp
Wal wal({ .file_path = "./.vix/wal.log" });

wal.append(record);
wal.replay(0, handler);
```

### 3. Outbox

Durable operation queue:

- enqueue
- claim
- complete
- retry

```cpp
auto id = outbox->enqueue(op, now);
outbox->claim(id, now);
outbox->complete(id, now);
```

### 4. RetryPolicy

Deterministic exponential backoff:

```cpp
RetryPolicy retry;
retry.base_delay_ms = 500;
retry.factor = 2.0;
retry.max_attempts = 8;
```

### 5. SyncWorker

Processes operations:

- fetch ready ops
- send via transport
- mark done / fail

### 6. SyncEngine

Orchestrates workers:

```cpp
SyncEngine engine(cfg, outbox, probe, transport);
engine.tick(now);
```

---

## Examples

Run examples with:

```bash
vix run examples/<file>.cpp
```

### Basic
```bash
vix run examples/01_outbox_basic.cpp
```

### Retry & failure
```bash
vix run examples/02_outbox_retry_and_failure.cpp
```

### WAL
```bash
vix run examples/03_wal_append_and_replay.cpp
```

### Worker
```bash
vix run examples/04_sync_worker_success.cpp
```

### Permanent failure
```bash
vix run examples/05_sync_worker_permanent_failure.cpp
```

### Engine requeue
```bash
vix run examples/06_sync_engine_requeue_inflight.cpp
```

### End-to-end (local-first)
```bash
vix run examples/07_local_write_wal_outbox_engine.cpp
```

### Offline to recovery
```bash
vix run examples/08_local_write_offline_then_recover.cpp
```

---

## Key invariant

If the process crashes at any point, the system can recover fully.

Because:

- WAL stores intent
- Outbox stores state
- Retry is deterministic

---

## When to use Vix Sync

Use it when you need:

- offline-first systems
- reliable message delivery
- distributed state convergence
- retry-safe APIs
- edge or unstable network environments

---

## What Vix Sync is NOT

- not a message broker
- not a queue system like Kafka
- not a distributed database

It is:

> a durable sync layer for real-world unreliable systems

---

## Future

- P2P transport
- conflict resolution
- multi-device sync
- edge replication

---

## License

MIT License
© Gaspard Kirira

