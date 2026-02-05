# Sync examples
This folder is a living playground for Vix.cpp offline-first sync.

Run with your usual workflow (example):

- vix run examples/sync/outbox_smoke.cpp
- vix run examples/sync/outbox_permanent_fail.cpp
- vix run examples/sync/inflight_timeout.cpp

Each example writes its own local outbox file under a .vix_example_sync_* folder.

Run:

- vix run examples/sync/vix_sync_showcase.cpp

It creates local files under:

- .vix_example_sync_smoke/
- .vix_example_sync_offline/
- .vix_example_sync_retry/
- .vix_example_sync_permfail/
- .vix_example_sync_inflight/
