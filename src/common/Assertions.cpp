#include "quartz/common/Assertions.h"

#include <cstdio>
#include <cstdlib>

namespace quartz {

[[noreturn]] void assertionFail(
    const char* expr,
    const std::string& msg,
    const char* file,
    int line,
    const char* func) noexcept {

    std::fprintf(stderr,
        "QUARTZ_ASSERT(%s) failed in %s at %s:%d: %s\n",
        expr, func, file, line, msg.c_str());
    std::fflush(stderr);
    std::abort();
}

} // namespace quartz
