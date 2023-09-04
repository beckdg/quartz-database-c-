# QuartzDB

A lightweight embedded binary database engine written in modern C++17.

**Version 1.0.0** — production-quality embedded storage with B-tree indexing, write-ahead logging, and crash recovery.

## Overview

QuartzDB is a structured binary database for applications that need fast, embedded storage without the overhead of an SQL database engine. It provides a layered architecture with durable WAL logging, checkpoint-based recovery, and a unified `Database` API.

## Features

- B-tree index with insert, delete, search, and range traversal
- Write-ahead logging with optional B-tree integration
- Checkpoint-based crash recovery and WAL replay
- Unified `Database` API (open, insert, erase, checkpoint, validate)
- Space management with extent allocation and fragmentation analysis
- Diagnostics (`ConsistencyChecker`, `IntegrityScanner`)
- Offline verification tool (`quartzdb_verify`)
- Microbenchmarks and fuzz harnesses
- CMake package config for embedding in downstream projects
- Fully offline build (vendored Catch2)

## Repository Layout

```
QuartzDB/
├── include/quartz/
│   ├── common/           # Status, Logger, Config, Version
│   ├── serialization/    # BinaryReader, BinaryWriter, Buffer
│   ├── storage/          # Page, DatabaseFile, PageAllocator
│   ├── format/           # DatabaseHeader, Superblock, PageReference
│   ├── pages/            # HeaderPage, DataPage, IndexPage, etc.
│   ├── space/            # SpaceManager, FreeSpaceMap, Extents
│   ├── btree/            # BTree, Key, LeafNode, InternalNode
│   ├── wal/              # LogManager, LogRecord, LogWriter
│   ├── recovery/         # RecoveryManager, CheckpointManager, LogReplayer
│   ├── core/             # Database, DatabaseOptions
│   ├── diagnostics/      # ConsistencyChecker, IntegrityScanner
│   ├── maintenance/      # BulkLoader, Vacuum
│   ├── metadata/         # Catalog, VersionCompatibility
│   └── instrumentation/  # Timer, Counter, Profiler
├── src/                  # Implementation
├── tests/                # Catch2 unit tests
├── examples/             # Example programs
├── benchmarks/           # Microbenchmarks
├── fuzz/                 # Fuzz targets (Clang libFuzzer)
├── tools/                # quartzdb_verify CLI
└── docs/                 # Architecture and guides
```

## Quick Start

```cpp
#include "quartz/core/Database.h"

quartz::core::Database db;
db.open("mydb.qdb", true);
db.insert(quartz::btree::Key::fromUInt32(42), pageRef);
db.checkpoint(false);
db.close();
```

## Build

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Options

| Option | Default | Description |
|--------|---------|-------------|
| `QUARTZDB_BUILD_TESTS` | ON | Unit tests |
| `QUARTZDB_BUILD_EXAMPLES` | ON | Example programs |
| `QUARTZDB_BUILD_BENCHMARKS` | OFF | Microbenchmarks |
| `QUARTZDB_BUILD_TOOLS` | ON | CLI tools (`quartzdb_verify`) |
| `QUARTZDB_BUILD_FUZZ` | OFF | Fuzz targets |
| `QUARTZDB_WARNINGS_AS_ERRORS` | OFF | Strict warnings |

## Testing

```bash
cmake -B build -DQUARTZDB_BUILD_TESTS=ON -DQUARTZDB_WARNINGS_AS_ERRORS=ON
cmake --build build
cd build && ctest --output-on-failure
```

## Documentation

- [Architecture](docs/architecture.md)
- [Database API](docs/database_api.md)
- [WAL](docs/write_ahead_log.md)
- [Recovery](docs/recovery.md)
- [Operations](docs/operations.md)
- [Contributing](docs/contributing.md)
- [Release Notes](RELEASE_NOTES.md)

## Current Status

**Version 1.0.0** — All core embedded storage subsystems complete. Record layer and SQL are planned for future releases.

## License

MIT License. See [LICENSE](LICENSE).
