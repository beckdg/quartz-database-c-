#include "quartz/serialization/BinaryReader.h"
#include "quartz/serialization/BinaryWriter.h"
#include "quartz/serialization/Buffer.h"
#include "quartz/wal/LogRecord.h"

#include <cstddef>
#include <cstdint>

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) || \
    (defined(__clang__) && defined(__has_feature) && __has_feature(address_sanitizer))
#define QUARTZ_HAS_LIBFUZZER 1
#endif

using namespace quartz;

#if defined(QUARTZ_HAS_LIBFUZZER)
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size < 4) return 0;
    serialization::Buffer buf;
    (void)buf.append(data, size);
    serialization::BinaryWriter writer(buf);
    wal::LogRecord record = wal::LogRecord::make(wal::LogRecordType::PageUpdate, 1);
    (void)record.serialize(writer);
    serialization::BinaryReader reader{serialization::BufferView(buf)};
    wal::LogRecord restored;
    (void)restored.deserialize(reader);
    return 0;
}
#else
int main() { return 0; }
#endif
