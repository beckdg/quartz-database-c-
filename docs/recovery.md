# WAL Recovery

QuartzDB 1.0 implements crash recovery through checkpointed B-tree snapshots and incremental WAL replay.

## Components

| Component | Role |
|-----------|------|
| `CheckpointPayload` | Serialized B-tree snapshot + metadata |
| `CheckpointManager` | Create, find, and restore checkpoints |
| `LogReplayer` | Apply logical WAL records to a B-tree |
| `RecoveryValidator` | Post-recovery consistency checks |
| `RecoveryManager` | Orchestrates full recovery and checkpoint API |

## Recovery Flow

1. Open the WAL file.
2. Scan for the last `CheckpointMarker` record.
3. If found, deserialize the embedded `CheckpointPayload` into the B-tree.
4. Replay all `PageUpdate` and `PageDelete` records with LSN strictly greater than the checkpoint LSN.
5. Restore writer LSN state from the on-disk log.
6. Validate tree structure and WAL ordering.

Structural records (`Allocation`, `Deallocation`, `NodeSplit`, `NodeMerge`) are logged for diagnostics and future page-level recovery but are not required for logical replay when a checkpoint snapshot exists.

## Checkpoint Format

The checkpoint payload contains:

- Magic (`CHPK`) and format version
- Checkpoint LSN
- Tree size and height
- Serialized B-tree bytes (`BTree::serialize`)

## Truncation

`CheckpointManager::truncateAfterCheckpoint` removes WAL bytes after a checkpoint LSN, enabling log compaction. Enable via `Database::checkpoint(true)` or `RecoveryManager::checkpoint` with `truncateWal = true`.

## Integration

`Database` runs recovery automatically on open when `DatabaseOptions::recoverOnOpen` is true (default). See [database_api.md](database_api.md).
