# B-Tree Node Engine

This document describes the B-Tree Node Engine (Phase 6a): local node representation, serialization, validation, and manipulation. Full tree algorithms (recursive insert, delete, root split/merge) are deferred to a later phase.

## Layer Position

```
Space Management
       ↓
B-Tree Node Engine   ← this document
       ↓
Future B-Tree Algorithms
```

The node layer depends on:

- `quartz::pages::IndexPage` — page ownership and on-disk layout
- `quartz::format::PageReference` — child/data page pointers
- `quartz::serialization::BinaryReader` / `BinaryWriter` — all wire encoding

It does **not** depend on transactions, recovery, the query engine, or record storage.

## Node Layout

Each B-tree node occupies one `IndexPage` (4096 bytes). The page payload (`IndexPageLayout`, 4032 bytes) is split into:

### Page-Level Header (IndexPageLayout)

| Field       | Size | Purpose                              |
|-------------|------|--------------------------------------|
| `nodeType`  | 4    | `1` = leaf, `2` = internal           |
| `keyCount`  | 4    | Number of separator/data keys        |
| `capacity`  | 4    | Maximum keys for this configuration  |
| `flags`     | 4    | `AllowDuplicates` (bit 0)            |
| `reserved`  | 24   | `reserved[0]` = node level           |
| `data`      | 3992 | Node body (header + entries)         |

### Node Body Header (start of `data`)

Written via `BinaryWriter`:

| Field            | Size | Purpose                          |
|------------------|------|----------------------------------|
| Magic            | 4    | `0x45525442` ("BTRE")            |
| `parent`         | 20   | `PageReference` to parent node   |
| `level`          | 4    | Distance from leaf (0 = leaf)      |
| `keyType`        | 1    | `KeyType` discriminator          |
| `allowDuplicates`| 1    | `0` or `1`                       |
| `binaryKeySize`  | 2    | Fixed binary key width           |

### Entry Area (after body header)

**Leaf entries** (repeated `keyCount` times):

```
Key | PageReference
```

Each `PageReference` points to a data page (record storage is a future phase). Leaves store keys and references only.

**Internal entries**:

```
PageReference leftChild
(Key | PageReference rightChild) × keyCount
```

Internal nodes store `keyCount` separator keys and `keyCount + 1` child references. The leftmost child precedes all keys.

## Key Ordering

Keys use a tagged representation (`KeyType`):

| Type    | Tag | Payload                         |
|---------|-----|---------------------------------|
| UInt32  | 1   | 4-byte unsigned integer         |
| UInt64  | 2   | 8-byte unsigned integer         |
| Binary  | 3   | `uint16` length + fixed bytes   |

Comparison rules:

1. Keys of different types are ordered by `KeyType` value (UInt32 < UInt64 < Binary).
2. Same-type keys use unsigned numeric or lexicographic byte order.
3. Binary keys must match the configured `binaryKeySize` on deserialize.

`KeyComparator` provides `lowerBound`, `upperBound`, and `find` for binary-search compatibility.

## Node Invariants

`NodeValidator` and per-node `validate()` check:

| Invariant | Leaf | Internal |
|-----------|------|----------|
| `keyCount ≤ capacity` | ✓ | ✓ |
| Keys sorted ascending | ✓ | ✓ |
| No duplicate keys (unless `AllowDuplicates`) | ✓ | ✓ |
| `keys.size() == keyCount` | ✓ | ✓ |
| `refs.size() == keyCount` | ✓ | — |
| `children.size() == keyCount + 1` | — | ✓ |
| Body magic valid | ✓ | ✓ |
| IndexPage layout valid | ✓ | ✓ |

## Serialization Format

### Key wire format

```
uint8  keyType
uint32 payload          (UInt32)
uint64 payload          (UInt64)
uint16 length + bytes   (Binary)
```

### Full page round-trip

`BTreeNode::serialize()` delegates to `IndexPage::serialize()`, writing the entire 4096-byte page. Node entries are synced into `IndexPageLayout::data` before serialization via `syncEntries()`.

### Entry-only serialization

`serializeBody()` / `deserializeBody()` operate on the entry area only (used for validation and testing).

## Cursor Model

`Cursor` operates within a **single node** in this phase:

- `bindLeaf()` / `bindInternal()` — attach to a node
- `seek(index)` / `seekKey(key)` — position
- `next()` / `previous()` — move within node
- `currentKey()` / `currentReference()` — access current entry
- `valid()` / `reset()` — state management

Future tree iterators will compose cursors with `SearchPath` for multi-level traversal.

## SearchPath

`SearchPath` is a stack of `SearchPathEntry` frames (`pageId`, `childIndex`, `pageRef`). It records parent chain state for future top-down traversal:

- `push()` / `pop()` / `clear()` / `reserve()`
- `depth()` / `parent()` / `top()` / `at()`

No tree traversal is performed in this phase.

## Split Planning (Future Algorithm)

`SplitMergePlanner::planLeafSplit()` and `planInternalSplit()` compute:

- `splitPosition` — index at which to divide entries (~50% fill)
- `promotedKey` — separator to push to parent
- `leftOccupancy` / `rightOccupancy` — post-split fill percentages
- `feasible` — whether split is meaningful (`0 < splitPos < keyCount`)

**Not executed in this phase.** Future tree code will:

1. Allocate a sibling page via `SpaceManager`
2. Move entries `[splitPosition, end)` to the sibling
3. Promote `promotedKey` into the parent internal node
4. Update parent/child `PageReference` links

## Merge Planning (Future Algorithm)

`planLeafMerge()` / `planInternalMerge()` compute:

- `combinedOccupancy` — fill if both nodes were merged
- `resultingKeyCount` — total keys after merge
- `feasible` — `combinedKeyCount ≤ capacity`

**Not executed in this phase.** Future tree code will:

1. Verify sibling underfill against a merge threshold
2. Pull sibling entries into the left node
3. Remove separator from parent
4. Release the sibling page to `SpaceManager`

## Public API Summary

| Component | Header | Role |
|-----------|--------|------|
| `Key` | `btree/Key.h` | Tagged key value |
| `KeyComparator` | `btree/KeyComparator.h` | Comparison and binary search |
| `BTreeNode` | `btree/BTreeNode.h` | Common node abstraction |
| `LeafNode` | `btree/LeafNode.h` | Ordered key + page ref storage |
| `InternalNode` | `btree/InternalNode.h` | Separator keys + child refs |
| `Cursor` | `btree/Cursor.h` | Single-node iterator |
| `SearchPath` | `btree/SearchPath.h` | Traversal stack |
| `NodeValidator` | `btree/NodeValidator.h` | Invariant checking |
| `BTreeStatistics` | `btree/BTreeStatistics.h` | Occupancy metrics |
| `SplitMergePlanner` | `btree/SplitMergePlanner.h` | Split/merge planning |

## Capacity Calculation

`BTreeNode::computeCapacity(config, type)` derives the maximum key count from:

- Available bytes in `data` minus body header
- Per-entry wire size: `Key` encoding + `PageReference` (20 bytes)
- Internal nodes reserve one additional left-child reference

UInt32 leaf nodes typically hold ~130+ entries per page; binary keys reduce capacity proportionally.
