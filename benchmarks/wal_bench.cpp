#include "quartz/instrumentation/Timer.h"
#include "quartz/wal/LogManager.h"
#include "quartz/wal/LogRecord.h"
#include "quartz/wal/LogTypes.h"

#include <cstdint>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

using namespace quartz;

int main() {
    const auto path = (fs::temp_directory_path() / "quartzdb_bench_wal").string();
    std::error_code ec;
    fs::remove(path, ec);

    wal::LogManager manager;
    if (!manager.initialize(path).ok()) {
        return 1;
    }

    instrumentation::Timer timer;
    timer.start();

    constexpr int kCount = 5000;
    for (int i = 0; i < kCount; ++i) {
        auto record = wal::LogRecord::make(wal::LogRecordType::PageUpdate, static_cast<storage::PageId>(i));
        if (!manager.append(std::move(record)).ok()) {
            return 1;
        }
    }
    if (!manager.flush().ok()) {
        return 1;
    }
    timer.stop();

    std::cout << "WAL append " << kCount << " records: " << timer.toString() << "\n";
    std::cout << "  bytesWritten=" << manager.statistics().bytesWritten << "\n";

    (void)manager.shutdown();
    fs::remove(path, ec);
    return 0;
}
