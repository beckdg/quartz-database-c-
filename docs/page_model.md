# Semantic Page Model

## Overview

The Semantic Page Layer provides strongly-typed C++ objects that wrap raw `storage::Page` instances with domain-specific semantics. This layer is the bridge between the generic storage and serialization layers and higher-level database components (B-tree, WAL, cache, record storage).

## Page Hierarchy

```
BasePage (abstract)
├── HeaderPage      (page 0 — database header + metadata)
├── FreeListPage    (free page ID collection)
├── DataPage        (generic payload + slot directory placeholder)
├── IndexPage       (B-tree node placeholder)
├── OverflowPage    (overflow chain link)
└── MetadataPage    (metadata section container)
```

## Responsibilities

### BasePage

- Owns a `storage::Page` via move semantics
- Exposes page metadata (id, type, size, payloadSize)
- Provides virtual `serialize()` / `deserialize()` using the existing serialization layer
- Provides pure virtual `validate()` and `clone()`
- Supports `reset()` (change page ID) and `clear()` (zero-fill buffer)
- Does NOT expose mutable raw byte pointers

### HeaderPage

- Wraps page 0, which contains the `DatabaseHeader`
- Stores: database header (128 bytes), superblock page ID, feature flags, format version, reserved region
- Provides typed accessors and mutators for all fields
- `initFromFormat()` initializes from a `format::DatabaseHeader`
- Used by: database initialization, format validation, startup recovery

### FreeListPage

- Tracks reusable page IDs
- Fixed capacity of 1000 entries per page
- Supports add, retrieve by index, clear, full/empty detection
- Used by: page allocator, free space management

### DataPage

- Generic data container with free space tracking
- Slot directory placeholder for future record storage
- `freeSpaceOffset` tracks the boundary between used and free space
- `availableSpace()` returns remaining bytes

### IndexPage

- Placeholder for future B-tree nodes
- Stores node type (leaf/internal), key count, capacity, flags
- No tree logic implemented
- Used by: future B-tree indexing

### OverflowPage

- Chain link for data that exceeds one page
- Holds next page reference and payload size
- `hasNextPage()` detects chain continuation
- `remainingCapacity()` computes available bytes

### MetadataPage

- Container for metadata entries
- Tracks entry count and version
- Raw data area reserved for future metadata implementations

## PageFactory

Central dispatch for creating the correct semantic page type:

```cpp
auto page = PageFactory::createPage(PageLayoutType::Header, 0);
auto page = PageFactory::createPage(std::move(rawPage));
auto page = PageFactory::deserialize(PageLayoutType::Data, reader);
```

- Uses `PageLayoutType` enum (parallels `storage::PageType` for supported types)
- Returns `nullptr` for unsupported/invalid types
- `deserialize()` creates a page, reads from `BinaryReader`, returns typed page

## PageValidator

Stateless validation utilities:

```cpp
auto st = PageValidator::validatePage(page);
auto st = PageValidator::validateLayoutType(type);
auto st = PageValidator::validateReservedFields(layout);
```

- Returns `Status::success()` or descriptive error
- Validates layout consistency, reserved field zeroing, page header integrity

## PageStatistics

Computes structural metadata about any page:

```cpp
auto stats = PageStatistics::compute(page);
std::cout << PageStatistics::toString(stats);
```

- `totalSize`, `headerOverhead`, `payloadSize`, `reservedBytes`
- `usedBytes`, `freeBytes`, `utilizationPercent`

## Relationships

### To Storage Layer

- Each semantic page owns a `storage::Page` by value (move-only)
- The `storage::Page` manages the 4096-byte buffer and 64-byte `PageHeader`
- The semantic page interprets the payload (4032 bytes) through a typed layout struct
- PageHeader type field determines which semantic type to use

### To Serialization Layer

- Pages serialize by writing their full 4096-byte buffer
- Deserialize by reading into the buffer and syncing the PageHeader
- Compatible with `BinaryWriter` / `BinaryReader` / `Buffer`
- Multiple pages can be serialized sequentially into one buffer

### To Format Layer

- `HeaderPage` contains a `DatabaseHeader` (128 bytes from format layer)
- Uses `Versioning::encodeVersion()` for format version encoding
- Format validation can be delegated to `FormatValidator`

### To Future Components

Future modules must depend on this layer:

- **B-tree**: Uses `IndexPage` for nodes, `DataPage` for leaf storage
- **Record storage**: Uses `DataPage` with slot directory
- **Cache/buffer pool**: Manages `BasePage` instances (or `unique_ptr<BasePage>`)
- **WAL/recovery**: Uses `OverflowPage` and `HeaderPage` for state tracking
- **Free space management**: Uses `FreeListPage` for tracking reusable pages

## Page Payload Layout

Each page has a 64-byte `PageHeader` followed by a 4032-byte payload.
The payload is interpreted through a packed struct unique to each page type:

| Page Type    | Payload Layout                          | Key Fields                            |
|-------------|----------------------------------------|---------------------------------------|
| HeaderPage  | `HeaderPageLayout`                     | DatabaseHeader (128), superblockPageId, flags, formatVersion, reserved[486] |
| FreeListPage| `FreeListPageLayout`                   | freeCount, capacity, reserved[3], freePages[1000] |
| DataPage    | `DataPageLayout`                       | freeSpaceOffset, slotCount, reserved1, reserved2[2], data[4008] |
| OverflowPage| `OverflowPageLayout`                   | nextPageId, payloadSize, reserved[2], data[4008] |
| IndexPage   | `IndexPageLayout`                      | nodeType, keyCount, capacity, flags, reserved[3], data[3992] |
| MetadataPage| `MetadataPageLayout`                   | entryCount, version, reserved[3], data[4000] |

All layouts are `#pragma pack(push, 1)` packed with `static_assert(sizeof(...) == kPagePayloadSize)`.

## Design Decisions

1. **Ownership**: Semantic pages own their `storage::Page` by value (move semantics), avoiding shared ownership complexity
2. **No raw byte exposure**: `BasePage` does not expose mutable raw pointers; typed accessors are the only mutation path
3. **Polymorphism via abstract base**: `BasePage` defines the interface; concrete types implement page-specific logic
4. **Factory dispatch**: `PageFactory` centralizes creation logic, avoiding scattered switch statements
5. **Serialization via raw bytes**: Pages serialize their entire buffer (header + payload) for simplicity and performance
6. **Reserved field validation**: `PageValidator` checks all reserved fields are zero, ensuring forward compatibility
