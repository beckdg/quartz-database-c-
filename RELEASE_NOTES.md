# QuartzDB 1.0.0 Release Notes

**Release date:** 2026

## Highlights

QuartzDB 1.0 is a production-quality embedded storage engine with durable write-ahead logging, crash recovery, and a unified database API.

## New Subsystems

### Recovery (Phase 8b)
- Checkpoint creation and restore
- WAL log replay (`PageUpdate`, `PageDelete`)
- `RecoveryManager`, `LogReplayer`, `CheckpointManager`
- Post-recovery validation

### Database API
- `quartz::core::Database` — open/close, insert/erase/find, checkpoint, validate
- `DatabaseOptions` runtime configuration
- `DatabaseStatistics` aggregate metrics

### Diagnostics
- `ConsistencyChecker` — cross-subsystem validation
- `IntegrityScanner` — offline file scan
- `DiagnosticReport` — structured findings

### Maintenance
- `BulkLoader` — sorted key bulk index build
- `Vacuum` — space reclamation analysis

### Metadata
- `Catalog` — schema and metadata descriptor registry
- `VersionCompatibility` — format version checks

### Instrumentation
- `Timer`, `Counter`, `Profiler`

### Tooling
- `quartzdb_verify` — offline database verification CLI
- Benchmarks: `bench_btree`, `bench_wal`, `bench_recovery`
- Fuzz targets: serialization, page, recovery

### Packaging
- CMake install rules and `QuartzDBConfig.cmake` package config

## Breaking Changes

- Version bumped to 1.0.0 (`config::kVersionMajor = 1`)
- `LogManager::restoreWriterState()` added (callers opening existing WAL files benefit automatically via `Database`)

## Known Limitations

- B-tree state is in-memory; checkpoints provide durability via WAL snapshots
- No SQL, transactions, replication, or networking
- Page checksums remain reserved fields
- Record/schema storage layer not yet implemented

## Upgrade Path

Databases created with 0.1.x format version remain readable when `VersionCompatibility::supportsFormat` returns true.
