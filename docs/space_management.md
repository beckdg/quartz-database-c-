# Space Management Layer

## Overview

The Space Management Layer provides page-level space allocation with contiguous extent support, policy-driven allocation strategies, and fragmentation analysis. It sits between the Semantic Page Layer and the Storage/Serialization Layer, exposing a unified interface for both single-page and extent-level allocation.

## Architecture

```
+---------------------------+
|    Semantic Page Layer    |
+---------------------------+
|   Space Management Layer  |  ← you are here
+---------------------------+
|  Storage & Serialization  |
+---------------------------+
```

The `SpaceManager` is the primary entry point. It wraps:
- `storage::PageAllocator` — individual page allocation (free-list reuse + sequential IDs)
- `ExtentAllocator` → `FreeSpaceMap` — contiguous range tracking with policy-driven allocation

## Components

### Extent (`include/quartz/space/Extent.h`)

A trivially copyable value type representing a contiguous range of pages:

```
struct Extent {
    storage::PageId start;   // First page in the extent
    std::uint32_t length;    // Number of pages
};
```

**Size:** 8 bytes (suitable for direct serialization via `BinaryWriter::write`/`BinaryReader::read`).

**Operations:**
- `contains(PageId)` / `contains(Extent)` — containment check
- `intersects(Extent)` — overlap test
- `adjacentTo(Extent)` — proximity check (touching but not overlapping)
- `canMerge(Extent)` — `intersects || adjacentTo`
- `merge(a, b)` — merge two extents (returns `std::optional`)
- `split(atPage)` — split at a page boundary (returns `std::optional<pair>`)
- `firstPage()`, `lastPage()`, `endPage()` — boundary accessors

### AllocationHints (`include/quartz/space/AllocationHints.h`)

Lightweight configuration struct for allocation decisions:

| Field               | Type     | Default | Description                           |
|---------------------|----------|---------|---------------------------------------|
| `preferContiguous`  | `bool`   | `false` | Prefer contiguous allocation          |
| `preferBeginning`   | `bool`   | `false` | Allocate from low page IDs            |
| `preferEnd`         | `bool`   | `false` | Allocate from high page IDs           |
| `minExtentLength`   | `uint32` | `1`     | Minimum acceptable extent size        |
| `exactFit`          | `bool`   | `false` | Must find an exact-length extent      |

### AllocationPolicy (`include/quartz/space/AllocationPolicy.h`)

Abstract strategy pattern for selecting free extents:

| Implementation    | Behavior                                                     |
|-------------------|--------------------------------------------------------------|
| `FirstFitPolicy`  | Returns the first free extent large enough to satisfy request |
| `BestFitPolicy`   | Returns the smallest free extent large enough to satisfy request |
| `SequentialPolicy`| Maintains a cursor; scans forward (with wrap-around) for free space |

### FreeSpaceMap (`include/quartz/space/FreeSpaceMap.h`)

Core data structure: a sorted, non-overlapping, non-adjacent vector of free `Extent`s.

**Key methods:**
- `initialize(start, length)` — set initial free range
- `markAllocated(range)` — remove a range from free space (splits extents as needed)
- `markFree(range)` — add a range to free space (merges adjacent extents)
- `findFreeRange(length, preferBeginning)` — find any extent >= length
- `findExactRange(length)` — find extent == length
- `findBestRange(length)` — find smallest extent >= length
- `countFree()` / `countFreeExtents()` — query counts
- `validate()` — assert sorted, non-overlapping, non-adjacent invariants

#### Allocation algorithm

When `markAllocated(range)` is called:
1. Binary search for the first overlapping extent
2. Four cases:
   - Range fully covers extent → remove extent
   - Extent fully contains range → split extent around range
   - Range overlaps end of extent → truncate extent
   - Range overlaps start of extent → shift extent start forward

#### Free algorithm

When `markFree(range)` is called:
1. Binary search for insertion point
2. Merge with left neighbor if adjacent/overlapping
3. Merge with right neighbors while adjacent/overlapping

### ExtentAllocator (`include/quartz/space/ExtentAllocator.h`)

Policy-driven wrapper around `FreeSpaceMap`:

```cpp
Extent allocate(uint32_t length, const AllocationHints& hints);
Status release(const Extent& extent);
```

### SpaceManager (`include/quartz/space/SpaceManager.h`)

Top-level coordinator. The primary API for higher layers.

**Single-page allocation:**
```
allocatePage() → PageAllocator::allocate() → FreeSpaceMap::markAllocated()
freePage(id)   → PageAllocator::free()      → FreeSpaceMap::markFree()
```

**Extent allocation:**
```
allocateExtent(n) → ExtentAllocator::allocate(n) → FreeSpaceMap::markAllocated()
freeExtent(ext)   → ExtentAllocator::release(ext) → FreeSpaceMap::markFree()
```

**Other:**
- `reserveRange(start, count)` — mark specific pages as reserved (not allocatable)
- Statistics tracking via `SpaceStatisticsCollector`
- Observable state: `allocatedPageCount()`, `freePageCount()`

### FragmentationAnalyzer (`include/quartz/space/FragmentationAnalyzer.h`)

Static analysis utilities:

```cpp
FragmentationReport report = FragmentationAnalyzer::analyze(fsm, totalPages);
```

| Metric                       | Definition                                              |
|------------------------------|---------------------------------------------------------|
| `fragmentationPercent`       | `100 * (1 - largestExtent / totalFreePages)`            |
| `largestFreeExtent`          | Maximum extent length                                   |
| `smallestFreeExtent`         | Minimum non-zero extent length                          |
| `averageFreeExtentSize`      | `totalFreePages / extentCount`                          |
| `allocationDensity`          | `usedPages / totalPages`                                |
| `maxContiguousAllocation`    | Largest single allocatable range                        |

### SpaceStatistics (`include/quartz/space/SpaceStatistics.h`)

Per-operation counters and snapshot state:

| Counter               | Description                          |
|-----------------------|--------------------------------------|
| `allocationsRequested`| Total allocation attempts             |
| `allocationsSucceeded`| Successful allocations                |
| `allocationsFailed`   | Failed allocations                    |
| `freesRequested`      | Total free attempts                   |
| `freesSucceeded`      | Successful frees                      |
| `freesFailed`         | Failed frees                          |
| `currentAllocatedPages` | Currently allocated page count      |
| `currentFreePages`    | Currently free page count             |
| `extentCount`         | Number of free extents                |
| `fragmentationPercent` | Calculated fragmentation level       |

## Design Decisions

1. **FreeSpaceMap does not duplicate PageAllocator's free list.** PageAllocator tracks individual freed pages for reuse (LIFO); FreeSpaceMap provides a contiguous-range view across the full managed address space. Both are kept in sync by SpaceManager.

2. **Extent is trivially copyable** (8 bytes). This enables direct serialization through `BinaryWriter::write`/`BinaryReader::read` without custom `SerializationTraits`.

3. **Policy pattern** allows allocation behavior to be swapped at runtime or configured per-database instance.

4. **No direct byte-pointer exposure** — all operations go through the public API.

5. **SpaceManager is the single entry point** for page/extent allocation. Future modules (Record Layer, B-tree) must request storage through SpaceManager rather than directly through PageAllocator.

## Initial State

On construction, `SpaceManager` initializes `FreeSpaceMap` with a single free extent covering all pages from `kFirstDataPageId` (8) through `kMaxPageId` (1,048,575), for a total of 1,048,568 managed pages. Reserved pages 0–7 (header, metadata, and future use) are never managed by the allocator.

## Future Extensions

- Persistent FreeSpaceMap serialization (save/restore allocation state on database open/close)
- Multi-file extent tracking
- Online defragmentation
- NUMA-aware allocation policies
- Custom allocator backends
