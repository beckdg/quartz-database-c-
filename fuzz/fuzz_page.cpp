#include "quartz/common/Config.h"
#include "quartz/storage/Page.h"

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION) || \
    (defined(__clang__) && defined(__has_feature) && __has_feature(address_sanitizer))
#define QUARTZ_HAS_LIBFUZZER 1
#endif

using namespace quartz;

#if defined(QUARTZ_HAS_LIBFUZZER)
extern "C" int LLVMFuzzerTestOneInput(const std::uint8_t* data, std::size_t size) {
    if (size < sizeof(storage::PageHeader)) return 0;
    storage::Page page(1, storage::PageType::Data);
    const auto copy = size < config::kPageSize ? size : config::kPageSize;
    std::memcpy(page.data(), data, copy);
    (void)page.isValid();
    return 0;
}
#else
int main() { return 0; }
#endif
