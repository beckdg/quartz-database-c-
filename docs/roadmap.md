# Development Roadmap

QuartzDB **1.0.0** is complete. This document records delivered phases and future work.

---

## Delivered in 1.0.0

### Foundation through B-tree (Phases 1–7)
- CMake build, Status, Logger, utilities, CI
- Storage layer, serialization, format specification
- Semantic pages, space management
- B-tree node engine and full algorithms

### Write-Ahead Log (Phase 8a)
- LogManager, LogRecord, LogWriter/Reader, LogValidator
- Optional BTreeWalAdapter integration

### Recovery (Phase 8b)
- CheckpointManager with B-tree snapshots
- LogReplayer for PageUpdate/PageDelete
- RecoveryManager and RecoveryValidator

### Database API & Tooling (Phase 9)
- `Database` lifecycle API
- Diagnostics, maintenance, metadata, instrumentation
- `quartzdb_verify` CLI
- Benchmarks and fuzz harnesses
- CMake install and package config

---

## Future Work

### Record Storage and Schema
- Fixed and variable-width record encoding
- Slot directory in DataPage
- Schema evolution

### Transactions
- Begin/commit/rollback
- Multi-record atomicity
- Savepoints

### Query API
- Iterator-based filters and projections
- Aggregate functions

### Compression and Checksums
- Per-page compression (LZ4/zstd)
- CRC32 checksum enforcement

### Advanced I/O
- Memory-mapped file backend
- Buffer pool / cache manager

### Replication
- Out of scope for embedded engine; not planned

---

Each future phase is designed to extend the existing layered architecture without breaking the dependency graph.
