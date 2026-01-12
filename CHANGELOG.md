# Changelog

All notable changes to this project will be documented in this file.

This project follows [Semantic Versioning](https://semver.org/).

---

## [0.1.0] — 2026-01-12

### Added

- Standalone offline-first synchronization engine.
- Write-Ahead Log (WAL) for durable operation tracking.
- Outbox abstraction with file-based storage.
- Retry policies and sync workers for resilient delivery.
- Public CMake target `vix::sync`.

### Changed

- Extracted `NetworkProbe` into the dedicated `net` module.
- Cleaned module boundaries and dependencies.
- Sync tests now link directly against `vix::sync`.

### Fixed

- Test build failures caused by missing transitive includes.
- CMake target linkage and include propagation issues.

### Notes

- This is the first public release of the Vix sync engine.
- Designed to operate under unreliable or offline network conditions.
