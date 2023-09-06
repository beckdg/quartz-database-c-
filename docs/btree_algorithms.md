# B-Tree Algorithms

This document describes the B-Tree Algorithms layer (Phase 7): complete insert, delete, search, and traversal built on the B-Tree Node Engine.

## Architecture

```
SpaceManager
     ↓
BTree (orchestration)
     ├── Rebalance (split, merge, borrow, rotate)
     ├── TreeValidator (whole-tree invariants)
     ├── TreeStatistics (aggregate metrics)
     ├── SearchPath (descent stack)
     └── Cursor (ordered traversal)
     ↓
LeafNode / InternalNode (existing node engine)
```

## Search Algorithm

Point lookup descends from the root using `InternalNode::lowerBound`:

1. At each internal node, find the child index `i` where `key[i-1] ≤ searchKey < key[i]`.
2. Follow `childAt(i)` to the next level.
3. At a leaf, use `find()` or `lowerBound()` for exact or bound search.

`SearchPath` records each `(pageId, childIndex)` frame during descent for split/merge parent updates.

**Complexity:** O(log n) node visits; O(log n) comparisons per node via binary search.

## Insert Algorithm

1. Descend to the target leaf using `lowerBound` (duplicate check if configured).
2. If the leaf has free capacity, insert locally via `LeafNode::insert`.
3. If the leaf is full:
   - Merge existing keys plus the new key into a sorted vector.
   - Split at `size / 2`; promote the middle key.
   - Assign left portion to the original leaf; allocate a sibling for the right portion.
   - Call `insertIntoParent` with the promoted key.
4. If the parent internal node is full, split it recursively (same midpoint policy).
5. If the root splits, allocate a new internal root with two children and increment tree height.

## Split Algorithm

Split policy (executed by `Rebalance`, planned by `SplitMergePlanner`):

| Step | Action |
|------|--------|
| 1 | Compute `splitIdx = keyCount / 2` after including the overflow key |
| 2 | Promote `keys[splitIdx]` to parent |
| 3 | Left node keeps `[0, splitIdx)` |
| 4 | Right sibling gets `(splitIdx, end)` |
| 5 | Allocate sibling page via `SpaceManager::allocatePage()` |

Root split creates a new internal root when `SearchPath` is empty after leaf split.

## Delete Algorithm

1. Descend to the leaf containing the key (exact match required).
2. Erase the entry locally via `LeafNode::eraseAt`.
3. If the leaf is underfull (`keyCount < capacity / 2`) and not the root:
   - Try **borrow from left** sibling (rotation).
   - Try **borrow from right** sibling.
   - **Merge** with a sibling if borrowing is not possible.
4. If the parent internal node becomes underfull, apply the same fix recursively.
5. If the root internal node has zero keys and one child, collapse the root.

## Merge Algorithm

Left merge (preferred when a left sibling exists):

1. Concatenate left sibling keys/refs with the current node.
2. Remove the parent separator via `InternalNode::eraseAt`.
3. Release the merged-away page through `SpaceManager::freePage()`.
4. Recurse upward if the parent underflows.

Internal node merge additionally pulls the parent separator key between merged children arrays.

## Cursor Traversal

`Cursor` supports both single-node and full-tree modes:

| Operation | Behavior |
|-----------|----------|
| `bindTree()` | Attach to a `BTree` instance |
| `seekBegin()` | Leftmost leaf, index 0 |
| `next()` | Advance within leaf, then next leaf in order |
| `previous()` | Retreat within leaf, then previous leaf |
| `end()` | Past-the-end sentinel |

Tree-mode traversal walks leaves in sorted key order without a separate leaf chain.

## Tree Invariants

`TreeValidator` checks:

- All leaves at the same depth
- Keys sorted within every node
- Separator bounds respected between parent and children
- Valid `PageReference` on every child pointer
- Local `NodeValidator` checks on each node
- Root consistency (single root page, valid type)

Minimum occupancy for non-root nodes: `capacity / 2` (floor).

## Complexity Analysis

| Operation | Time | Notes |
|-----------|------|-------|
| Search | O(log n) | Height × binary search per node |
| Insert | O(log n) | Descent + possible split propagation |
| Delete | O(log n) | Descent + possible merge/borrow propagation |
| Cursor next | O(log n) amortized | May climb parent chain between leaves |
| Validate | O(n) | Visits every node |

## Future Persistence Integration

The current `BTree::serialize()` writes root metadata and all node pages via existing `IndexPage` serialization. Future work will:

- Persist root `PageId` in `SchemaDescriptor`
- Load nodes through `DatabaseFile` on demand
- Integrate with WAL and transactions (Phase 8+)

Pages are always allocated and released through `SpaceManager`; no manual buffer manipulation.
