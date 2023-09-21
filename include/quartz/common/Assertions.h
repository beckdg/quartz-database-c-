#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>

namespace quartz {

enum class AssertAction {
    Abort,
    Break  // Reserved for future debugger integration
};

#ifdef QUARTZDB_DEBUG
#define QUARTZ_ASSERT(expr, msg)                                                \
    do {                                                                        \
        if (!(expr)) {                                                          \
            ::quartz::assertionFail(                                            \
                #expr, msg, __FILE__, __LINE__, __FUNCTION__);                   \
        }                                                                       \
    } while (false)
#else
#define QUARTZ_ASSERT(expr, msg) ((void)0)
#endif

[[noreturn]] void assertionFail(
    const char* expr,
    const std::string& msg,
    const char* file,
    int line,
    const char* func) noexcept;

} // namespace quartz
