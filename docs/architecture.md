# QuartzDB Architecture

## High-Level Module Overview

The database is organized into layered modules with strict dependency direction. Higher layers depend only on lower layers — never the reverse.

```
+-----------------------------+
|        Query API            |   Future: SQL, filters, projections
+-----------------------------+
|      Transaction Layer      |   Future: ACID, concurrency control
+-----------------------------+
|    Database API (core)      |   v1.0: Database, lifecycle, statistics
+-----------------------------+
|    Diagnostics/Maintenance  |   v1.0: ConsistencyChecker, BulkLoader, Vacuum
+-----------------------------+
|    Recovery Layer           |   v1.0: CheckpointManager, LogReplayer, RecoveryManager
+-----------------------------+
|    Write-Ahead Log          |   v1.0: LogManager, durable append/flush/read
+-----------------------------+
|    B-Tree Algorithms        |   BTree, insert/delete, rebalance, traversal
+-----------------------------+
|    B-Tree Node Engine       |   Key, LeafNode, InternalNode, Cursor
+-----------------------------+
|       Record Layer          |   Future: schema, record storage, encoding
+-----------------------------+
|    Semantic Page Layer      |   Typed page wrappers (HeaderPage, DataPage, etc.)
+-----------------------------+
|   Space Management Layer    |   Extents, FreeSpaceMap, SpaceManager
+-----------------------------+
|    Binary Format Layer      |   Database header, superblock, descriptors
+-----------------------------+
|  Storage & Serialization    |   Page management, binary serialization
+-----------------------------+
|  Utilities & Common Layer   |   Status, Logging, Endian, etc.
+-----------------------------+
```

## Layer Responsibilities

### Utilities & Common Layer (include/quartz/common/, include/quartz/util/)

Provides foundational types and reusable utilities across all modules.

- `Version` — Build and version identification
- `Status` — Error propagation without exceptions
- `Logger` — Structured diagnostic output
- `ScopeGuard` — RAII scope-based cleanup
- `NonCopyable` / `NonMovable` — Inheritance mixins for resource-managing classes
- `Assertions` — Internal invariant checking
- `Endian` — Byte-order utilities
- `FileUtils` — Cross-platform filesystem operations

### Serialization Layer (include/quartz/serialization/)

Provides reusable binary serialization infrastructure used by nearly every higher layer.

- `Buffer` — Owning, resizable byte buffer with RAII and move semantics
- `BufferView` — Non-owning immutable view into a buffer
- `VariableLengthInteger` — VarInt encoding/decoding (unsigned and signed ZigZag variants)
- `BinaryReader` — Bounds-checked binary deserialization with endian support
- `BinaryWriter` — Dynamic-growth binary serialization with endian support
- `SerializationContext` — Version, endianness, and strict-mode configuration
- `SerializationTraits` — Template-based serialization customization point (e.g., `std::string`)
- `Serializer` — Version-aware serialization with format header (magic + version + size)

All operations return `Status`. No exceptions used for control flow within the library.

### Format Layer (include/quartz/format/)

Defines the persistent on-disk structures that form the binary specification of QuartzDB.

- `DatabaseHeader` — Persistent database header (magic, version, page size, UUID, timestamps)
- `Superblock` — Global metadata (page counts, allocation state, free page tracking)
- `ObjectId` — 128-bit UUID (RFC 4122 version 4 random) with string conversion and comparison
- `PageReference` — Lightweight page pointer (page ID, generation, type, flags)
- `FeatureFlags` — Strongly typed 64-bit feature flag bitfield with bit operations
- `Compatibility` — Version/feature negotiation and forward/backward compatibility checking
- `Versioning` — Format version constants and validation
- `MagicNumbers` — Canonical magic number constants across the format
- `MetadataDescriptor` — Describes a metadata section (location, size, type, version)
- `SchemaDescriptor` — Describes a schema section (identifier, version, location)
- `FormatValidator` — Validation utilities that check headers, versions, flags, and consistency

All format structures integrate with the serialization layer via `SerializationTraits` specializations.

### Semantic Page Layer (include/quartz/pages/)

Provides strongly-typed page objects that wrap raw `storage::Page` instances with domain-specific semantics.

- `BasePage` — Abstract base class owning a `storage::Page` with serialization, validation, clone, and reset
- `HeaderPage` — Page 0 wrapper containing `DatabaseHeader`, superblock reference, feature flags, format version
- `FreeListPage` — Page tracking reusable page IDs with capacity management
- `DataPage` — Generic data page with free space tracking and slot directory placeholder
- `IndexPage` — Future B-tree node page with key/capacity/node type metadata
- `OverflowPage` — Overflow chain page with next-page reference and payload tracking
- `MetadataPage` — Metadata section container with entry count and version
- `PageFactory` — Central dispatch for creating/deserializing the correct page type
- `PageValidator` — Validation utilities for all page types and reserved fields
- `PageStatistics` — Computation of page utilization, overhead, and free space

All page types integrate with the serialization layer and can round-trip through `BinaryWriter`/`BinaryReader`.
Future components (B-tree, Record Layer, Cache, WAL) must depend on this layer rather than directly manipulating raw `storage::Page` objects.

### Space Management Layer (include/quartz/space/)

Provides page-level space allocation, contiguous extent management, and fragmentation analysis. Coordinates between the page-level `PageAllocator` and higher-level storage consumers.

- `Extent` — Trivially copyable value type representing a contiguous range of pages (start, length) with intersection, merge, split, and adjacency operations
- `FreeSpaceMap` — Tracks available page ranges as sorted, non-overlapping extents; supports marking ranges allocated/free, finding contiguous blocks by policy
- `AllocationPolicy` — Abstract strategy for selecting free extents: `FirstFitPolicy`, `BestFitPolicy`, `SequentialPolicy`
- `AllocationHints` — Options struct (preferContiguous, exactFit, preferBeginning/End)
- `ExtentAllocator` — Policy-driven allocator wrapping FreeSpaceMap with allocate/release operations
- `SpaceManager` — Top-level coordinator wrapping `storage::PageAllocator` (single pages) and `ExtentAllocator` (extents); keeps both in sync, tracks statistics via `SpaceStatisticsCollector`
- `FragmentationAnalyzer` — Static analysis: fragmentation percent, largest/smallest free extent, allocation density, recommendations
- `SpaceStatistics` / `SpaceStatisticsCollector` — Allocation/free counters, current usage, fragmentation tracking

All space management types compose rather than inherit. The `SpaceManager` is the primary entry point; future modules (Record Layer, B-tree) request pages through it.

### B-Tree Node Engine (include/quartz/btree/)

Provides local B-tree node representation, serialization, validation, and manipulation. Does not implement recursive tree algorithms.

- `Key` — Tagged key value (uint32, uint64, fixed binary) with comparison, serialization, hashing
- `KeyComparator` — Ascending comparison and binary-search helpers
- `BTreeNode` — Common abstraction backed by `IndexPage` (level, capacity, parent ref, occupancy)
- `LeafNode` — Ordered keys with `PageReference` values; local insert/erase/search
- `InternalNode` — Separator keys with child `PageReference` array; local insert/erase
- `Cursor` — Single-node seek/next/previous traversal
- `SearchPath` — Stack of parent frames for future tree traversal
- `NodeValidator` — Sorted-key, capacity, and serialization integrity checks
- `BTreeStatistics` — Key count, occupancy, free slots, utilization
- `SplitMergePlanner` — Split/merge feasibility planning (no execution)

All nodes own an `IndexPage` and serialize through `BinaryReader`/`BinaryWriter`. See [btree_nodes.md](btree_nodes.md).

### B-Tree Algorithms (include/quartz/btree/)

Implements complete B-tree operations on top of the node engine.

- `BTree` — Full tree with `insert`, `erase`, `find`, `contains`, `lowerBound`, `upperBound`, `validate`, `statistics`, `Cursor` iteration
- `Rebalance` — Split, merge, borrow, and rotation execution
- `TreeValidator` — Whole-tree invariant validation (depth, ordering, separators)
- `TreeStatistics` — Height, node counts, occupancy, operation counters
- Extended `Cursor` — Full-tree forward and reverse traversal

Pages allocated via `SpaceManager`; see [btree_algorithms.md](btree_algorithms.md).

### Write-Ahead Log (include/quartz/wal/)

Provides durable append-only logging independent of recovery logic.

- `LogSequenceNumber` — Monotonic LSN with comparison, serialization, and invalid sentinel
- `LogRecord` — Typed binary record with page id, timestamp, payload, reserved fields
- `LogBuffer` — In-memory batching with synchronous flush
- `LogFile` — Journal file backed by `DatabaseFile` with aligned records
- `LogWriter` / `LogReader` — Append, batch, flush, seek, and iteration
- `LogValidator` — Header, LSN ordering, and integrity validation
- `LogStatistics` — Record counts, bytes written/read, flush and buffer metrics
- `LogManager` — Top-level WAL coordinator
- `BTreeWalAdapter` — Optional `BTreeWalSink` implementation for tree operations

The B-tree depends only on the abstract `BTreeWalSink`; WAL depends on B-tree types for the adapter. See [write_ahead_log.md](write_ahead_log.md).

### Recovery Layer (include/quartz/recovery/)

Implements checkpoint-based crash recovery and WAL replay.

- `CheckpointPayload` — Serialized B-tree snapshot in checkpoint records
- `CheckpointManager` — Create, find, restore, and truncate checkpoints
- `LogReplayer` — Apply logical WAL records to a B-tree
- `RecoveryValidator` — Post-recovery consistency verification
- `RecoveryManager` — Full recovery orchestration

See [recovery.md](recovery.md).

### Database API (include/quartz/core/)

Top-level embedded storage engine entry point.

- `Database` — Open/close, insert/erase/find, checkpoint, validate
- `DatabaseOptions` — Runtime configuration (WAL, recovery, buffer size)
- `DatabaseStatistics` — Aggregate subsystem metrics
- `DatabaseContext` — Non-owning subsystem view

See [database_api.md](database_api.md).

### Diagnostics (include/quartz/diagnostics/)

- `ConsistencyChecker` — Cross-subsystem validation
- `IntegrityScanner` — Offline page file scan
- `DiagnosticReport` — Structured findings

### Maintenance (include/quartz/maintenance/)

- `BulkLoader` — Sorted bulk index construction
- `Vacuum` — Space reclamation analysis

### Metadata (include/quartz/metadata/)

- `Catalog` — Schema and metadata descriptor registry
- `VersionCompatibility` — Format version negotiation

### Instrumentation (include/quartz/instrumentation/)

- `Timer` — High-resolution elapsed time
- `Counter` — Thread-safe monotonic counter
- `Profiler` — Named timers and counters

### I/O Abstraction Layer (include/quartz/io/ — future)

Abstracts the storage medium behind a uniform interface. Supports pluggable backends:

- `FileBackend` — Memory-mapped or pread/pwrite file access
- `MemoryBackend` — In-memory store for testing

### Binary Format Layer (future)

Defines the exact on-disk layout of pages, records, and metadata. Versioned and self-describing.

### Cache Layer (future)

Manages a buffer pool of fixed-size pages. Implements page eviction, pinning, and dirty-page tracking.

### Record Layer (future)

Translates between in-memory record representations and the binary storage format. Schema-aware.

### Index Layer (future)

Provides full B-tree algorithms (recursive insert/delete, root split/merge, point lookup, range scan) built on the B-Tree Node Engine.

### Transaction Layer (future)

Implements ACID semantics via write-ahead logging (WAL) and snapshot isolation.

### Query API (future)

Exposes iterator-based record retrieval with optional filtering and projection.

## Design Principles

1. **No global mutable state** — All state is explicitly owned and passed through dependency injection.
2. **RAII everywhere** — Resources (memory, file handles, locks) are always wrapped in owning objects.
3. **Error handling via Status** — Exceptions are not used for control flow within the library itself.
4. **Const correctness** — Methods are marked `const` wherever mutation is unnecessary.
5. **Testability** — Every layer depends on abstractions that can be mocked or replaced for testing.
6. **Fuzzability** — Entry points are designed to accept arbitrary binary input for fuzzing.
