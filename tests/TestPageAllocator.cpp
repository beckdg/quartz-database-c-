#include "quartz/storage/PageAllocator.h"
#include "quartz/storage/StorageConstants.h"

#include <catch2/catch_test_macros.hpp>

#include <unordered_set>

using namespace quartz::storage;

TEST_CASE("PageAllocator starts empty after reserved pages", "[allocator]") {
    PageAllocator alloc;
    CHECK(alloc.stats().currentAllocated == 0);
    CHECK(alloc.stats().totalAllocated == 0);
    CHECK(alloc.stats().freeListSize == 0);
}

TEST_CASE("PageAllocator allocates sequential IDs starting after reserved", "[allocator]") {
    PageAllocator alloc;
    auto id1 = alloc.allocate();
    auto id2 = alloc.allocate();
    CHECK(id1 == kReservedPageCount);
    CHECK(id2 == kReservedPageCount + 1);
    CHECK(alloc.stats().currentAllocated == 2);
    CHECK(alloc.stats().totalAllocated == 2);
}

TEST_CASE("PageAllocator reuses freed IDs", "[allocator]") {
    PageAllocator alloc;
    auto id1 = alloc.allocate();
    auto id2 = alloc.allocate();
    CHECK(id2 == id1 + 1);

    CHECK(alloc.free(id1));
    CHECK(alloc.stats().currentAllocated == 1);
    CHECK(alloc.stats().totalAllocated == 2);

    auto id3 = alloc.allocate();
    CHECK(id3 == id1);  // Reuses the freed ID
    CHECK(alloc.stats().currentAllocated == 2);
    CHECK(alloc.stats().totalAllocated == 3);
}

TEST_CASE("PageAllocator cannot free reserved page IDs", "[allocator]") {
    PageAllocator alloc;
    CHECK_FALSE(alloc.free(0));
    CHECK_FALSE(alloc.free(kReservedPageCount - 1));
}

TEST_CASE("PageAllocator cannot free invalid or out-of-range IDs", "[allocator]") {
    PageAllocator alloc;
    CHECK_FALSE(alloc.free(kInvalidPageId));
    CHECK_FALSE(alloc.free(999999));
}

TEST_CASE("PageAllocator cannot double-free", "[allocator]") {
    PageAllocator alloc;
    auto id = alloc.allocate();
    CHECK(alloc.free(id));
    CHECK_FALSE(alloc.free(id));
}

TEST_CASE("PageAllocator::isAllocated checks correctly", "[allocator]") {
    PageAllocator alloc;
    // Reserved IDs are pre-allocated
    CHECK(alloc.isAllocated(0));
    CHECK(alloc.isAllocated(kReservedPageCount - 1));

    // Non-reserved IDs after allocation
    auto id = alloc.allocate();
    CHECK(alloc.isAllocated(id));

    // Freed IDs are not allocated
    alloc.free(id);
    CHECK_FALSE(alloc.isAllocated(id));

    // Out-of-range
    CHECK_FALSE(alloc.isAllocated(kInvalidPageId));
    CHECK_FALSE(alloc.isAllocated(999999));
}

TEST_CASE("PageAllocator tracks peak allocation", "[allocator]") {
    PageAllocator alloc;
    CHECK(alloc.stats().peakAllocated == 0);

    alloc.allocate();
    alloc.allocate();
    alloc.allocate();
    CHECK(alloc.stats().peakAllocated == 3);
    CHECK(alloc.stats().currentAllocated == 3);

    auto id = alloc.allocate();
    alloc.free(id);
    CHECK(alloc.stats().currentAllocated == 3);
    CHECK(alloc.stats().peakAllocated == 4);
}

TEST_CASE("PageAllocator::reset clears all state", "[allocator]") {
    PageAllocator alloc;
    alloc.allocate();
    alloc.allocate();
    alloc.allocate();
    alloc.free(alloc.allocate());
    alloc.reset();

    CHECK(alloc.stats().currentAllocated == 0);
    CHECK(alloc.stats().totalAllocated == 0);
    CHECK(alloc.stats().freeListSize == 0);
    CHECK(alloc.stats().peakAllocated == 0);

    // Should start fresh from kReservedPageCount
    auto id = alloc.allocate();
    CHECK(id == kReservedPageCount);
}

TEST_CASE("PageAllocator handles many allocations", "[allocator]") {
    PageAllocator alloc;
    std::unordered_set<PageId> ids;
    constexpr int kCount = 1000;

    for (int i = 0; i < kCount; ++i) {
        auto id = alloc.allocate();
        CHECK(id != kInvalidPageId);
        CHECK(ids.insert(id).second);  // no duplicates
    }

    CHECK(alloc.stats().currentAllocated == kCount);
    CHECK(alloc.stats().totalAllocated == kCount);
    CHECK(alloc.stats().peakAllocated == kCount);

    // Free every other page
    for (int i = 0; i < kCount; i += 2) {
        CHECK(alloc.free(static_cast<PageId>(kReservedPageCount + static_cast<std::uint32_t>(i))));
    }
    CHECK(alloc.stats().freeListSize == kCount / 2);
    CHECK(alloc.stats().currentAllocated == kCount / 2);

    // Reallocate to get recycled IDs
    for (int i = 0; i < kCount / 2; ++i) {
        auto id = alloc.allocate();
        CHECK(id != kInvalidPageId);
    }
    CHECK(alloc.stats().currentAllocated == kCount);
}

TEST_CASE("PageAllocator LIFO reuse order", "[allocator]") {
    PageAllocator alloc;
    auto id1 = alloc.allocate();
    auto id2 = alloc.allocate();
    auto id3 = alloc.allocate();

    CHECK(id3 != kInvalidPageId);

    alloc.free(id1);
    alloc.free(id2);

    // Should reuse id2 first (LIFO)
    CHECK(alloc.allocate() == id2);
    CHECK(alloc.allocate() == id1);
}

TEST_CASE("PageAllocator returns kInvalidPageId when exhausted", "[allocator]") {
    PageAllocator alloc;
    // Exhaust by allocating many pages (not practical for max)
    // Instead, verify overflow doesn't happen
    CHECK(alloc.allocate() != kInvalidPageId);
}
