# Operations Guide

## Database Files

| File | Purpose |
|------|---------|
| `*.qdb` | Main database file (header page + data pages) |
| `*.qdb.wal` | Write-ahead log |

## Backup

1. Call `Database::checkpoint()` to embed a consistent snapshot in the WAL.
2. Copy both `.qdb` and `.qdb.wal` files.

## Recovery

Recovery runs automatically on `Database::open()` when WAL is enabled. Manual recovery:

```cpp
recovery::RecoveryManager::recover(wal, tree, result);
```

## Verification

Use the offline verification tool:

```bash
quartzdb_verify /path/to/database.qdb
```

Or programmatically:

```cpp
db.validate();
diagnostics::ConsistencyChecker::analyze(db);
diagnostics::IntegrityScanner scanner(db.file());
scanner.scan();
```

## Maintenance

- **Checkpoint**: `db.checkpoint(truncateWal)` — embeds B-tree snapshot; optionally truncates WAL.
- **Vacuum**: `maintenance::Vacuum(space).run()` — refreshes space statistics.
- **Bulk load**: `maintenance::BulkLoader(tree).loadSorted(keys, refs)` — fast sorted index build.

## Monitoring

`Database::statistics()` exposes B-tree, space, and WAL counters. Enable verbose logging via `Logger::setLevel(Logger::Level::Debug)`.
