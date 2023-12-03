#include "quartz/space/Extent.h"
#include "quartz/space/AllocationHints.h"
#include "quartz/space/AllocationPolicy.h"
#include "quartz/space/FreeSpaceMap.h"
#include "quartz/space/ExtentAllocator.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/space/SpaceStatistics.h"
#include "quartz/space/FragmentationAnalyzer.h"

#include <catch2/catch_test_macros.hpp>

#include <type_traits>

using namespace quartz::space;
using namespace quartz::storage;

// ===== Extent =====

TEST_CASE("Extent default construction is invalid", "[space][extent]") {
    Extent e;
    CHECK_FALSE(e.isValid());
    CHECK(e.start == kInvalidPageId);
    CHECK(e.length == 0);
}

TEST_CASE("Extent valid construction", "[space][extent]") {
    Extent e{100, 5};
    CHECK(e.isValid());
    CHECK(e.start == 100);
    CHECK(e.length == 5);
    CHECK(e.firstPage() == 100);
    CHECK(e.lastPage() == 104);
    CHECK(e.endPage() == 105);
}

TEST_CASE("Extent contains page", "[space][extent]") {
    Extent e{100, 5};
    CHECK(e.contains(100));
    CHECK(e.contains(104));
    CHECK_FALSE(e.contains(99));
    CHECK_FALSE(e.contains(105));
}

TEST_CASE("Extent contains other extent", "[space][extent]") {
    Extent outer{100, 20};
    Extent inner{105, 5};
    Extent partial{105, 20};
    Extent outside{200, 5};

    CHECK(outer.contains(inner));
    CHECK_FALSE(outer.contains(partial));
    CHECK_FALSE(outer.contains(outside));
}

TEST_CASE("Extent intersects", "[space][extent]") {
    Extent a{100, 10};
    Extent b{105, 10};
    Extent c{200, 10};
    Extent d{110, 5};  // touches end of a

    CHECK(a.intersects(b));
    CHECK(b.intersects(a));
    CHECK_FALSE(a.intersects(c));
    CHECK_FALSE(a.intersects(d));  // a ends at 110, d starts at 110 — not intersecting
    CHECK_FALSE(d.intersects(a));
}

TEST_CASE("Extent adjacentTo", "[space][extent]") {
    Extent a{100, 10};
    Extent b{110, 10};  // adjacent
    Extent c{111, 10};  // not adjacent
    Extent d{90, 10};   // adjacent on left

    CHECK(a.adjacentTo(b));
    CHECK(b.adjacentTo(a));
    CHECK_FALSE(a.adjacentTo(c));
    CHECK(a.adjacentTo(d));
    CHECK(d.adjacentTo(a));
}

TEST_CASE("Extent canMerge", "[space][extent]") {
    Extent a{100, 10};
    Extent b{110, 10};  // adjacent
    Extent c{108, 10};  // overlapping
    Extent d{200, 10};  // separate

    CHECK(a.canMerge(b));
    CHECK(a.canMerge(c));
    CHECK_FALSE(a.canMerge(d));
}

TEST_CASE("Extent merge", "[space][extent]") {
    Extent a{100, 10};
    Extent b{110, 10};
    Extent c{105, 10};

    auto merged = Extent::merge(a, b);
    CHECK(merged.has_value());
    CHECK(merged->start == 100);
    CHECK(merged->length == 20);

    merged = Extent::merge(a, c);
    CHECK(merged.has_value());
    CHECK(merged->start == 100);
    CHECK(merged->length == 15);

    merged = Extent::merge(a, Extent{200, 5});
    CHECK_FALSE(merged.has_value());
}

TEST_CASE("Extent split", "[space][extent]") {
    Extent e{100, 10};

    auto split = e.split(103);
    CHECK(split.has_value());
    CHECK(split->first.start == 100);
    CHECK(split->first.length == 3);
    CHECK(split->second.start == 103);
    CHECK(split->second.length == 7);

    // Split at start boundary — invalid
    CHECK_FALSE(e.split(100).has_value());
    // Split at end boundary — invalid
    CHECK_FALSE(e.split(110).has_value());
    // Split outside — invalid
    CHECK_FALSE(e.split(50).has_value());
}

TEST_CASE("Extent comparison", "[space][extent]") {
    Extent a{100, 5};
    Extent b{100, 5};
    Extent c{200, 5};

    CHECK(a == b);
    CHECK(a < c);
    CHECK_FALSE(c < a);
}

TEST_CASE("Extent invalid operations", "[space][extent]") {
    Extent invalid;
    Extent valid{100, 5};

    CHECK_FALSE(invalid.intersects(valid));
    CHECK_FALSE(invalid.adjacentTo(valid));
    CHECK_FALSE(Extent::merge(invalid, valid).has_value());
    CHECK_FALSE(invalid.split(50).has_value());
}

// ===== FreeSpaceMap =====

TEST_CASE("FreeSpaceMap initialize and query", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 100);

    CHECK(fsm.countFree() == 100);
    CHECK(fsm.countFreeExtents() == 1);
    CHECK(fsm.contains(100));
    CHECK(fsm.contains(199));
    CHECK_FALSE(fsm.contains(99));
    CHECK_FALSE(fsm.contains(200));
}

TEST_CASE("FreeSpaceMap clear", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 100);
    fsm.clear();
    CHECK(fsm.countFree() == 0);
    CHECK(fsm.countFreeExtents() == 0);
}

TEST_CASE("FreeSpaceMap markAllocated — beginning", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 10);

    auto st = fsm.markAllocated(Extent{100, 3});
    CHECK(st.ok());
    CHECK(fsm.countFree() == 7);
    CHECK(fsm.countFreeExtents() == 1);
    CHECK(fsm.extents()[0].start == 103);

    CHECK_FALSE(fsm.contains(100));
    CHECK_FALSE(fsm.contains(102));
    CHECK(fsm.contains(103));
}

TEST_CASE("FreeSpaceMap markAllocated — middle", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 10);

    auto st = fsm.markAllocated(Extent{103, 4});
    CHECK(st.ok());
    CHECK(fsm.countFree() == 6);
    CHECK(fsm.countFreeExtents() == 2);
    CHECK(fsm.extents()[0].start == 100);
    CHECK(fsm.extents()[0].length == 3);
    CHECK(fsm.extents()[1].start == 107);
    CHECK(fsm.extents()[1].length == 3);
}

TEST_CASE("FreeSpaceMap markAllocated — end", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 10);

    auto st = fsm.markAllocated(Extent{107, 3});
    CHECK(st.ok());
    CHECK(fsm.countFree() == 7);
    CHECK(fsm.countFreeExtents() == 1);
    CHECK(fsm.extents()[0].start == 100);
    CHECK(fsm.extents()[0].length == 7);
}

TEST_CASE("FreeSpaceMap markAllocated — full range", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 10);

    auto st = fsm.markAllocated(Extent{100, 10});
    CHECK(st.ok());
    CHECK(fsm.countFree() == 0);
    CHECK(fsm.countFreeExtents() == 0);
}

TEST_CASE("FreeSpaceMap markAllocated — subsumes multiple extents", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    // Create two separate free extents: [0,30), [50,80)
    REQUIRE(fsm.markAllocated(Extent{30, 20}).ok());

    CHECK(fsm.countFreeExtents() == 2);
    CHECK(fsm.countFree() == 60);

    // Allocate range [20, 60) — overlaps both
    auto st = fsm.markAllocated(Extent{20, 40});
    CHECK(st.ok());
    // Remaining: [0,20), [60,80)
    CHECK(fsm.countFree() == 40);
    CHECK(fsm.countFreeExtents() == 2);
    CHECK(fsm.extents()[0].start == 0);
    CHECK(fsm.extents()[0].length == 20);
    CHECK(fsm.extents()[1].start == 60);
    CHECK(fsm.extents()[1].length == 20);
}

TEST_CASE("FreeSpaceMap markAllocated — invalid extent", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    auto st = fsm.markAllocated(Extent{});
    CHECK_FALSE(st.ok());

    st = fsm.markAllocated(Extent{200, 10});  // outside free range
    CHECK_FALSE(st.ok());
}

TEST_CASE("FreeSpaceMap markFree — merges with left", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    // Create: [0,30), [70,100)
    REQUIRE(fsm.markAllocated(Extent{30, 40}).ok());
    CHECK(fsm.countFreeExtents() == 2);
    CHECK(fsm.countFree() == 60);

    // Free [25,35): overlaps left extent [0,30) and frees [30,35) from allocated
    auto st = fsm.markFree(Extent{25, 10});
    CHECK(st.ok());
    CHECK(fsm.countFreeExtents() == 2);
    CHECK(fsm.extents()[0].start == 0);
    CHECK(fsm.extents()[0].length == 35);
    CHECK(fsm.extents()[1].start == 70);
    CHECK(fsm.extents()[1].length == 30);
    CHECK(fsm.countFree() == 65);
}

TEST_CASE("FreeSpaceMap markFree — creates new extent", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 50);

    // Allocate the entire range
    REQUIRE(fsm.markAllocated(Extent{100, 50}).ok());
    CHECK(fsm.countFree() == 0);

    // Free a middle range
    auto st = fsm.markFree(Extent{120, 10});
    CHECK(st.ok());
    CHECK(fsm.countFree() == 10);
    CHECK(fsm.countFreeExtents() == 1);
    CHECK(fsm.extents()[0].start == 120);
}

TEST_CASE("FreeSpaceMap findFreeRange", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 50);

    // Create extents: [100,120), [130,150)
    REQUIRE(fsm.markAllocated(Extent{120, 10}).ok());
    CHECK(fsm.countFreeExtents() == 2);

    auto ext = fsm.findFreeRange(10, true);
    CHECK(ext.start == 100);
    CHECK(ext.length == 10);

    ext = fsm.findFreeRange(15, true);
    CHECK(ext.start == 100);
    CHECK(ext.length == 15);

    // Request >20 pages: both extents are only 20 pages, so it should fail
    ext = fsm.findFreeRange(25, true);
    CHECK_FALSE(ext.isValid());

    // Find from end (preferBeginning=false)
    ext = fsm.findFreeRange(15, false);
    CHECK(ext.start == 135);  // 150-15 = 135
    CHECK(ext.length == 15);

    ext = fsm.findFreeRange(50, true);
    CHECK_FALSE(ext.isValid());
}

TEST_CASE("FreeSpaceMap findExactRange", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 50);

    // Create extents: [100,115) len=15, [115,150) allocated, then check
    REQUIRE(fsm.markAllocated(Extent{125, 10}).ok());

    auto ext = fsm.findExactRange(15);
    CHECK(ext.isValid());
    CHECK(ext.start == 100);
    CHECK(ext.length == 15);

    ext = fsm.findExactRange(7);
    CHECK_FALSE(ext.isValid());
}

TEST_CASE("FreeSpaceMap findBestRange", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    REQUIRE(fsm.markAllocated(Extent{10, 5}).ok());   // [0,10) len=10, [15,100) len=85
    REQUIRE(fsm.markAllocated(Extent{20, 70}).ok());  // [0,10) len=10, [15,20) len=5, [90,100) len=10

    auto ext = fsm.findBestRange(8);
    CHECK(ext.isValid());
    // best fit for 8: first extent len=10 (closest fit >= 8), not len=5 (too small)
    CHECK(ext.start == 0);
    CHECK(ext.length == 8);
}

TEST_CASE("FreeSpaceMap validate", "[space][fsm]") {
    FreeSpaceMap fsm;

    // Empty map is valid
    CHECK(fsm.validate().ok());

    fsm.initialize(0, 100);
    CHECK(fsm.validate().ok());

    REQUIRE(fsm.markAllocated(Extent{30, 20}).ok());
    CHECK(fsm.validate().ok());

    REQUIRE(fsm.markFree(Extent{30, 10}).ok());
    CHECK(fsm.validate().ok());
}

TEST_CASE("FreeSpaceMap allocate then free then re-allocate", "[space][fsm]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    CHECK(fsm.countFree() == 100);

    REQUIRE(fsm.markAllocated(Extent{10, 20}).ok());
    CHECK(fsm.countFree() == 80);

    REQUIRE(fsm.markFree(Extent{10, 20}).ok());
    CHECK(fsm.countFree() == 100);
    CHECK(fsm.countFreeExtents() == 1);  // merged back

    REQUIRE(fsm.markAllocated(Extent{10, 20}).ok());
    CHECK(fsm.countFree() == 80);
}

// ===== AllocationPolicy =====

TEST_CASE("FirstFitPolicy allocates from first suitable extent", "[space][policy]") {
    FreeSpaceMap fsm;
    fsm.initialize(100, 50);
    // Two extents: [100,120), [130,150)
    REQUIRE(fsm.markAllocated(Extent{120, 10}).ok());

    FirstFitPolicy policy;
    AllocationHints hints;
    hints.preferContiguous = true;

    auto ext = policy.allocate(fsm, 10, hints);
    CHECK(ext.isValid());
    CHECK(ext.start == 100);
}

TEST_CASE("BestFitPolicy finds smallest sufficient extent", "[space][policy]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);
    // Create extents: [0,10), [15,20), [90,100)
    REQUIRE(fsm.markAllocated(Extent{10, 5}).ok());
    REQUIRE(fsm.markAllocated(Extent{20, 70}).ok());

    BestFitPolicy policy;
    AllocationHints hints;
    hints.preferContiguous = true;

    // Request 5 pages — best fit is [15,20) len=5
    auto ext = policy.allocate(fsm, 5, hints);
    CHECK(ext.isValid());
    CHECK(ext.start == 15);
    CHECK(ext.length == 5);
}

TEST_CASE("SequentialPolicy allocates from cursor", "[space][policy]") {
    FreeSpaceMap fsm;
    fsm.initialize(8, 100);

    SequentialPolicy policy;
    AllocationHints hints;

    auto ext = policy.allocate(fsm, 5, hints);
    CHECK(ext.isValid());
    CHECK(ext.start == 8);
    CHECK(ext.length == 5);
}

TEST_CASE("Policy name returns correct string", "[space][policy]") {
    FirstFitPolicy ff;
    BestFitPolicy bf;
    SequentialPolicy sq;

    CHECK(ff.name() == "FirstFit");
    CHECK(bf.name() == "BestFit");
    CHECK(sq.name() == "Sequential");
}

// ===== ExtentAllocator =====

TEST_CASE("ExtentAllocator construction", "[space][allocator]") {
    ExtentAllocator alloc(std::make_unique<FirstFitPolicy>());
    alloc.freeSpaceMap().initialize(8, 100);

    CHECK(alloc.freePageCount() == 100);
    CHECK(alloc.freeExtentCount() == 1);
}

TEST_CASE("ExtentAllocator allocate and release", "[space][allocator]") {
    ExtentAllocator alloc(std::make_unique<FirstFitPolicy>());
    alloc.freeSpaceMap().initialize(8, 100);

    AllocationHints hints;
    hints.preferContiguous = true;

    auto ext = alloc.allocate(10, hints);
    CHECK(ext.isValid());
    CHECK(ext.start == 8);
    CHECK(ext.length == 10);
    CHECK(alloc.freePageCount() == 90);

    auto st = alloc.release(ext);
    CHECK(st.ok());
    CHECK(alloc.freePageCount() == 100);
}

TEST_CASE("ExtentAllocator setPolicy", "[space][allocator]") {
    ExtentAllocator alloc(std::make_unique<FirstFitPolicy>());
    CHECK(alloc.policy().name() == "FirstFit");

    alloc.setPolicy(std::make_unique<BestFitPolicy>());
    CHECK(alloc.policy().name() == "BestFit");
}

TEST_CASE("ExtentAllocator clear", "[space][allocator]") {
    ExtentAllocator alloc(std::make_unique<FirstFitPolicy>());
    alloc.freeSpaceMap().initialize(8, 100);
    alloc.allocate(10);
    alloc.clear();

    CHECK(alloc.freePageCount() == 0);
    CHECK(alloc.freeExtentCount() == 0);
}

TEST_CASE("ExtentAllocator release invalid extent", "[space][allocator]") {
    ExtentAllocator alloc(std::make_unique<FirstFitPolicy>());
    auto st = alloc.release(Extent{});
    CHECK_FALSE(st.ok());
}

// ===== SpaceStatistics =====

TEST_CASE("SpaceStats records allocations", "[space][stats]") {
    SpaceStats stats;
    CHECK(stats.allocationsRequested == 0);

    stats.recordAllocation(true);
    CHECK(stats.allocationsRequested == 1);
    CHECK(stats.allocationsSucceeded == 1);
    CHECK(stats.currentAllocatedPages == 1);

    stats.recordAllocation(false);
    CHECK(stats.allocationsRequested == 2);
    CHECK(stats.allocationsFailed == 1);
    CHECK(stats.currentAllocatedPages == 1);  // unchanged on failure
}

TEST_CASE("SpaceStats records frees", "[space][stats]") {
    SpaceStats stats;
    stats.currentAllocatedPages = 5;

    stats.recordFree(true);
    CHECK(stats.freesRequested == 1);
    CHECK(stats.freesSucceeded == 1);
    CHECK(stats.currentAllocatedPages == 4);

    stats.recordFree(false);
    CHECK(stats.freesRequested == 2);
    CHECK(stats.freesFailed == 1);
    CHECK(stats.currentAllocatedPages == 4);  // unchanged on failure
}

TEST_CASE("SpaceStatisticsCollector reset", "[space][stats]") {
    SpaceStatisticsCollector collector;
    collector.recordAllocation(true);
    collector.reset();
    CHECK(collector.stats().allocationsRequested == 0);
}

// ===== FragmentationAnalyzer =====

TEST_CASE("FragmentationAnalyzer empty map", "[space][frag]") {
    FreeSpaceMap fsm;
    fsm.initialize(8, 100);

    auto report = FragmentationAnalyzer::analyze(fsm, 100 + 8);
    CHECK(report.totalFreePages == 100);
    CHECK(report.largestFreeExtent == 100);
    CHECK(report.fragmentationPercent == 0.0);
    CHECK(report.extentCount == 1);
}

TEST_CASE("FragmentationAnalyzer fragmented map", "[space][frag]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);

    // Create 5 equal extents of 10 pages each
    REQUIRE(fsm.markAllocated(Extent{10, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{30, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{50, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{70, 10}).ok());
    // Remaining: [0,10), [20,30), [40,50), [60,70), [80,100)
    // Total free: 50 pages, 5 extents

    auto frag = FragmentationAnalyzer::calculateFragmentation(fsm, 100);
    CHECK(frag > 0.0);
    // Largest free extent = 20 ([80,100) or [0,10) — wait [80,100) is 20)
    // frag = 100*(1 - 20/50) = 100*0.6 = 60%
    CHECK(frag > 59.0);
    CHECK(frag < 61.0);

    auto largest = FragmentationAnalyzer::largestFreeExtent(fsm);
    CHECK(largest == 20);

    auto avg = FragmentationAnalyzer::averageFreeExtentSize(fsm);
    CHECK(avg > 9.0);
    CHECK(avg < 11.0);

    auto density = FragmentationAnalyzer::allocationDensity(fsm, 100);
    CHECK(density > 0.49);
    CHECK(density < 0.51);
}

TEST_CASE("FragmentationAnalyzer fully allocated", "[space][frag]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);
    REQUIRE(fsm.markAllocated(Extent{0, 100}).ok());

    auto frag = FragmentationAnalyzer::calculateFragmentation(fsm, 100);
    CHECK(frag == 0.0);

    auto total = FragmentationAnalyzer::totalFreePages(fsm);
    CHECK(total == 0);
}

TEST_CASE("FragmentationAnalyzer generates recommendation", "[space][frag]") {
    FreeSpaceMap fsm;
    fsm.initialize(0, 100);
    REQUIRE(fsm.markAllocated(Extent{10, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{30, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{50, 10}).ok());
    REQUIRE(fsm.markAllocated(Extent{70, 10}).ok());

    auto report = FragmentationAnalyzer::analyze(fsm, 100);
    CHECK(report.fragmentationPercent > 25.0);
    CHECK(!report.recommendation.empty());
}

// ===== SpaceManager =====

TEST_CASE("SpaceManager construction initializes free space", "[space][manager]") {
    SpaceManager sm;
    CHECK(sm.freePageCount() > 0);
    CHECK(sm.allocatedPageCount() == 0);
    CHECK(sm.statistics().stats().allocationsRequested == 0);
}

TEST_CASE("SpaceManager allocatePage", "[space][manager]") {
    SpaceManager sm;
    auto id = sm.allocatePage();
    CHECK(id != kInvalidPageId);
    CHECK(id >= kFirstDataPageId);
    CHECK(sm.freePageCount() == (kMaxPageId + 1 - kFirstDataPageId - 1));

    // Allocate another
    auto id2 = sm.allocatePage();
    CHECK(id2 != id);
}

TEST_CASE("SpaceManager allocatePage and freePage round trip", "[space][manager]") {
    SpaceManager sm;
    auto id = sm.allocatePage();
    CHECK(id != kInvalidPageId);

    auto freeBefore = sm.freePageCount();
    CHECK(sm.freePage(id));
    CHECK(sm.freePageCount() == freeBefore + 1);
}

TEST_CASE("SpaceManager allocateExtent", "[space][manager]") {
    SpaceManager sm;
    AllocationHints hints;
    hints.preferContiguous = true;

    auto ext = sm.allocateExtent(10, hints);
    CHECK(ext.isValid());
    CHECK(ext.length == 10);
    CHECK(ext.start >= kFirstDataPageId);
}

TEST_CASE("SpaceManager allocateExtent and freeExtent", "[space][manager]") {
    SpaceManager sm;
    AllocationHints hints;
    hints.preferContiguous = true;

    auto ext = sm.allocateExtent(10, hints);
    CHECK(ext.isValid());

    auto beforeFree = sm.freePageCount();
    auto st = sm.freeExtent(ext);
    CHECK(st.ok());
    CHECK(sm.freePageCount() == beforeFree + 10);
}

TEST_CASE("SpaceManager reserveRange", "[space][manager]") {
    SpaceManager sm;
    auto before = sm.freePageCount();

    auto st = sm.reserveRange(1000, 50);
    CHECK(st.ok());
    CHECK(sm.freePageCount() == before - 50);
}

TEST_CASE("SpaceManager statistics reflect allocations", "[space][manager]") {
    SpaceManager sm;
    CHECK(sm.statistics().stats().allocationsRequested == 0);

    sm.allocatePage();
    CHECK(sm.statistics().stats().allocationsRequested == 1);
    CHECK(sm.statistics().stats().allocationsSucceeded == 1);

    sm.allocateExtent(5, AllocationHints{});
    CHECK(sm.statistics().stats().allocationsRequested == 2);
    CHECK(sm.statistics().stats().allocationsSucceeded == 2);
}

TEST_CASE("SpaceManager reset returns to initial state", "[space][manager]") {
    SpaceManager sm;
    sm.allocatePage();
    sm.allocateExtent(10, AllocationHints{});
    sm.reset();

    CHECK(sm.statistics().stats().allocationsRequested == 0);
    CHECK(sm.freePageCount() > 0);
}

TEST_CASE("SpaceManager with SequentialPolicy", "[space][manager]") {
    auto sm = SpaceManager(std::make_unique<SequentialPolicy>());
    auto id1 = sm.allocatePage();
    auto id2 = sm.allocatePage();
    CHECK(id2 > id1);
}

TEST_CASE("SpaceManager freePage fails for invalid ID", "[space][manager]") {
    SpaceManager sm;
    CHECK_FALSE(sm.freePage(0));   // reserved
    CHECK_FALSE(sm.freePage(kInvalidPageId));
}

TEST_CASE("SpaceManager reserveRange fails for reserved range", "[space][manager]") {
    SpaceManager sm;
    auto st = sm.reserveRange(0, 5);
    CHECK_FALSE(st.ok());
}

TEST_CASE("SpaceManager freePage then allocate reuses ID via PageAllocator", "[space][manager]") {
    SpaceManager sm;
    auto id = sm.allocatePage();
    CHECK(sm.freePage(id));

    auto id2 = sm.allocatePage();
    // PageAllocator reuses freed pages (LIFO), so id2 should equal id
    CHECK(id2 == id);
}

// ===== Integration =====

TEST_CASE("Extent serialization — trivially copyable", "[space][integration]") {
    CHECK(std::is_trivially_copyable_v<Extent>);
    // Extent should be 8 bytes: 4 byte PageId + 4 byte uint32_t
    CHECK(sizeof(Extent) == 8);
}

TEST_CASE("Multiple allocations and frees maintain consistency", "[space][integration]") {
    SpaceManager sm;
    auto initialFree = sm.freePageCount();
    CHECK(sm.allocatedPageCount() == 0);

    // Allocate 10 single pages
    PageId ids[10];
    for (int i = 0; i < 10; ++i) {
        ids[i] = sm.allocatePage();
        CHECK(ids[i] != kInvalidPageId);
    }

    // Allocate 2 extents
    AllocationHints hints;
    hints.preferContiguous = true;
    auto ext1 = sm.allocateExtent(5, hints);
    auto ext2 = sm.allocateExtent(8, hints);

    CHECK(sm.allocatedPageCount() == 23);
    CHECK(sm.freePageCount() == initialFree - 23);

    // Free half the single pages
    for (int i = 0; i < 5; ++i) {
        CHECK(sm.freePage(ids[i]));
    }

    // Free one extent
    CHECK(sm.freeExtent(ext1).ok());

    CHECK(sm.allocatedPageCount() == 13);
    CHECK(sm.freePageCount() == initialFree - 13);

    // Free remaining
    for (int i = 5; i < 10; ++i) {
        CHECK(sm.freePage(ids[i]));
    }
    CHECK(sm.freeExtent(ext2).ok());

    CHECK(sm.freePageCount() == initialFree);
    CHECK(sm.allocatedPageCount() == 0);
}

TEST_CASE("Auxiliary and detail tests compile cleanly", "[space][aux]") {
    // Verify AllocationHints default values
    AllocationHints hints;
    CHECK_FALSE(hints.preferContiguous);
    CHECK_FALSE(hints.preferBeginning);
    CHECK_FALSE(hints.preferEnd);
    CHECK(hints.minExtentLength == 1);
    CHECK_FALSE(hints.exactFit);
}
