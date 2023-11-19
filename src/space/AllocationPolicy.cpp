#include "quartz/space/AllocationPolicy.h"
#include "quartz/space/FreeSpaceMap.h"

namespace quartz {
namespace space {

// ── FirstFitPolicy ────────────────────────────────────────────

Extent FirstFitPolicy::allocate(FreeSpaceMap& fsm, std::uint32_t length,
                                const AllocationHints& hints) {
    if (hints.exactFit) {
        return fsm.findExactRange(length);
    }
    if (hints.preferContiguous || length > 1) {
        return fsm.findFreeRange(length, !hints.preferEnd);
    }
    return fsm.findFreeRange(length, true);
}

// ── BestFitPolicy ─────────────────────────────────────────────

Extent BestFitPolicy::allocate(FreeSpaceMap& fsm, std::uint32_t length,
                               const AllocationHints& hints) {
    if (hints.exactFit) {
        return fsm.findExactRange(length);
    }
    if (hints.preferContiguous || length > 1) {
        auto best = fsm.findBestRange(length);
        if (best.isValid()) return best;
        return fsm.findFreeRange(length, true);
    }
    return fsm.findFreeRange(length, true);
}

// ── SequentialPolicy ──────────────────────────────────────────

Extent SequentialPolicy::allocate(FreeSpaceMap& fsm, std::uint32_t length,
                                  const AllocationHints& hints) {
    (void)hints;

    // Search from cursor forward for a range of free pages
    const auto& extents = fsm.extents();
    if (extents.empty()) return {};

    auto it = std::lower_bound(extents.begin(), extents.end(), Extent{cursor_, 0});
    if (it == extents.end()) {
        it = extents.begin();
    }

    // Check extent at or after cursor
    if (it != extents.end() && it->length >= length) {
        cursor_ = it->start;
        return Extent{cursor_, length};
    }

    // Search the remaining extents
    for (auto scan = it; scan != extents.end(); ++scan) {
        if (scan->length >= length) {
            cursor_ = scan->start;
            return Extent{cursor_, length};
        }
    }

    // Wrap around and search from beginning
    for (auto scan = extents.begin(); scan != it; ++scan) {
        if (scan->length >= length) {
            cursor_ = scan->start;
            return Extent{cursor_, length};
        }
    }

    return {};
}

} // namespace space
} // namespace quartz
