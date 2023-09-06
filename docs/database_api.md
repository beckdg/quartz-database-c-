# Database API

The `quartz::core::Database` class is the top-level entry point for embedded applications.

## Opening a Database

```cpp
#include "quartz/core/Database.h"

quartz::core::DatabaseOptions options;
options.enableWal = true;
options.recoverOnOpen = true;

quartz::core::Database db(options);
auto st = db.open("/path/to/database.qdb", true);  // create if missing
```

The companion WAL file is stored at `<path>.wal`.

## Operations

```cpp
db.insert(quartz::btree::Key::fromUInt32(42), pageRef);
db.find(key, outRef);
db.erase(key);
db.checkpoint(false);   // write B-tree snapshot to WAL
db.validate();          // cross-subsystem consistency check
db.close();
```

## Configuration

`DatabaseOptions` controls runtime behavior:

| Field | Default | Description |
|-------|---------|-------------|
| `enableWal` | `true` | Enable write-ahead logging |
| `recoverOnOpen` | `true` | Run recovery on open |
| `truncateWalOnCheckpoint` | `false` | Truncate WAL after checkpoint (also when `checkpoint(true)` is called) |
| `walBufferCapacity` | 65536 | In-memory WAL buffer size |
| `btreeConfig` | UInt32 keys | B-tree node configuration |

## Statistics

`db.statistics()` returns aggregate metrics from the B-tree, space manager, and WAL subsystems.

## Subsystem Access

Advanced users can access subsystems directly:

```cpp
db.tree();   // B-tree index
db.space();  // Space manager
db.wal();    // Log manager
db.file();   // Database file
db.context(); // Non-owning DatabaseContext view
```

## Lifecycle

1. Construct `Database` with options.
2. `open()` — initializes storage, WAL, and recovery.
3. Perform operations.
4. `checkpoint()` periodically for fast recovery.
5. `close()` — flushes WAL and closes files.
