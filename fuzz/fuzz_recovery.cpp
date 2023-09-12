#include "quartz/btree/BTree.h"
#include "quartz/recovery/LogReplayer.h"
#include "quartz/space/SpaceManager.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogRecord.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) || \
    (defined(__clang__) && defined(__has_feature) && __has_feature(address_sanitizer))
#define QUARTZ_HAS_LIBFUZZER 1
#endif

namespace fs = std::filesystem;
using namespace quartz;

#if defined(QUARTZ_HAS_LIBFUZZER)
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    (void)data;
    (void)size;
    const auto path = (fs::temp_directory_path() / "quartzdb_fuzz_recovery.wal").string();
    wal::LogManager wal;
    if (!wal.initialize(path).ok()) return 0;
    space::SpaceManager space;
    auto tree = btree::BTree::create(space);
    recovery::RecoveryResult result;
    (void)recovery::LogReplayer::replay(wal, tree, wal::LogSequenceNumber::invalid(), result);
    wal.shutdown();
    std::error_code ec;
    fs::remove(path, ec);
    return 0;
}
#else
int main() { return 0; }
#endif
