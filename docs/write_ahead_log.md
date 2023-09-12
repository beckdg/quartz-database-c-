# Write-Ahead Log (WAL)

Phase 8a introduces durable logging infrastructure for QuartzDB. Recovery, transactions, and checkpoint replay are deferred to a later phase.

## Architecture

The WAL sits above the B-tree layer in the dependency graph:

```
B-Tree → Write-Ahead Log → Recovery (future)
```

Components are separated by responsibility:

| Component | Role |
|-----------|------|
| `LogSequenceNumber` | Monotonic LSN type with ordering and serialization |
| `LogRecord` | Binary record header + variable payload |
| `LogBuffer` | In-memory batching before file I/O |
| `LogFile` | Append-only journal backed by `DatabaseFile` |
| `LogWriter` | LSN assignment, serialization, buffering |
| `LogReader` | Sequential iteration, seek, validation |
| `LogValidator` | Header, LSN ordering, and integrity checks |
| `LogStatistics` | Write/read/flush counters and buffer usage |
| `LogManager` | Top-level coordinator |
| `BTreeWalAdapter` | Implements `BTreeWalSink` for optional tree integration |

## WAL File Layout

```
+------------------+
| WalFileHeader    |  16 bytes (magic, version, pageSize, reserved)
+------------------+
| Record 0         |  uint32 size + serialized LogRecord + padding to 8-byte alignment
+------------------+
| Record 1         |
+------------------+
| ...              |
+------------------+
```

The header uses `MagicNumbers::kJournalMagic` and `kWalFormatVersion`.

## Log Record Layout

Each record serializes through `BinaryWriter`:

| Field | Type | Notes |
|-------|------|-------|
| type | `uint8` | `LogRecordType` discriminator |
| lsn | `uint64` | Assigned by `LogWriter` |
| timestamp | `uint64` | Microseconds since steady clock epoch |
| pageId | `PageId` | Associated page, if any |
| transactionId | `uint64` | Placeholder for future transaction manager |
| payloadLength | `uint32` | Byte length of payload |
| checksum | `uint32` | Placeholder (currently 0) |
| reserved0 | `uint32` | Future use |
| reserved1 | `uint32` | Future use |
| payload | bytes | Type-specific data |

### Record Types

- `PageCreate`, `PageUpdate`, `PageDelete`
- `NodeSplit`, `NodeMerge`
- `Allocation`, `Deallocation`
- `MetadataUpdate`, `CheckpointMarker`

New types can be appended to `LogRecordType` without changing existing values.

## LSN Design

- Invalid LSN: `0`
- First assigned LSN: `1`
- Strictly monotonic per `LogWriter` instance
- `LogValidator::validateLsnOrdering` enforces increasing sequence on read

## Buffering

`LogBuffer` accumulates serialized records in memory. `LogWriter::flush` drains the buffer to `LogFile` and calls `DatabaseFile::flush`. No background threads or asynchronous I/O.

## B-Tree Integration

`BTreeWalSink` is an abstract interface in `include/quartz/btree/`. The B-tree calls it optionally when:

- Inserting or erasing keys (`PageUpdate`, `PageDelete`)
- Allocating or freeing pages (`Allocation`, `Deallocation`)
- Splitting or merging nodes (`NodeSplit`, `NodeMerge`)

`BTreeWalAdapter` in `include/quartz/wal/` implements the sink using `LogManager`. WAL is **optional** — tree correctness does not depend on logging.

```cpp
LogManager manager;
manager.initialize("database.wal");
BTreeWalAdapter adapter(manager);

BTree tree = BTree::create(space, config);
tree.setWalSink(&adapter);
tree.insert(key, value);  // emits WAL records when sink is set
manager.flush();
```

## Future Recovery Process

Recovery is implemented in v1.0 — see [recovery.md](recovery.md). The recovery layer:

1. Scans the WAL for the last `CheckpointMarker`
2. Restores the embedded B-tree snapshot
3. Replays subsequent `PageUpdate`/`PageDelete` records
4. Validates tree and WAL ordering

Future work: page-level replay, transaction commit boundaries, and automatic background checkpointing.

## Future Checkpoints

`CheckpointMarker` records reserve a slot for periodic truncation. Checkpoint recovery will record a consistent LSN boundary so older log segments can be discarded after pages are flushed to the main database file.
