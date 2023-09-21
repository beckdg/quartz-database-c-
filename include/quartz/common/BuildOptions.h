#pragma once

#include <cstdint>

namespace quartz {
namespace build {

// ── Build Configuration ──────────────────────────────────────

#if defined(QUARTZDB_DEBUG)
inline constexpr bool kDebugBuild = true;
inline constexpr bool kReleaseBuild = false;
#elif defined(NDEBUG)
inline constexpr bool kDebugBuild = false;
inline constexpr bool kReleaseBuild = true;
#else
inline constexpr bool kDebugBuild = false;
inline constexpr bool kReleaseBuild = false;
#endif

inline constexpr bool kAssertionsEnabled =
#if defined(QUARTZDB_DEBUG)
    true
#else
    false
#endif
    ;

// ── Sanitizers ───────────────────────────────────────────────

inline constexpr bool kAddressSanitizerEnabled =
#if defined(__SANITIZE_ADDRESS__)
    true
#else
    false
#endif
    ;

inline constexpr bool kUndefinedBehaviorSanitizerEnabled =
#if defined(__SANITIZE_UNDEFINED__)
    true
#else
    false
#endif
    ;

// ── Compiler Detection ───────────────────────────────────────

#if defined(__clang__)
inline constexpr const char* kCompilerName = "Clang";
inline constexpr int kCompilerMajor = __clang_major__;
inline constexpr int kCompilerMinor = __clang_minor__;
#elif defined(__GNUC__) || defined(__GNUG__)
inline constexpr const char* kCompilerName = "GCC";
inline constexpr int kCompilerMajor = __GNUC__;
inline constexpr int kCompilerMinor = __GNUC_MINOR__;
#elif defined(_MSC_VER)
inline constexpr const char* kCompilerName = "MSVC";
inline constexpr int kCompilerMajor = _MSC_VER / 100;
inline constexpr int kCompilerMinor = _MSC_VER % 100;
#else
inline constexpr const char* kCompilerName = "Unknown";
inline constexpr int kCompilerMajor = 0;
inline constexpr int kCompilerMinor = 0;
#endif

inline constexpr const char* kCompilerVersionString =
#if defined(__clang__)
    __clang_version__
#elif defined(__GNUC__) || defined(__GNUG__)
    __VERSION__
#elif defined(_MSC_VER)
    "MSVC " ## _MSC_VER
#else
    "Unknown"
#endif
    ;

// ── Platform Detection ───────────────────────────────────────

#if defined(_WIN32) || defined(_WIN64)
inline constexpr const char* kPlatformName = "Windows";
inline constexpr bool kPlatformWindows = true;
inline constexpr bool kPlatformLinux = false;
inline constexpr bool kPlatformMacOS = false;
#elif defined(__linux__)
inline constexpr const char* kPlatformName = "Linux";
inline constexpr bool kPlatformWindows = false;
inline constexpr bool kPlatformLinux = true;
inline constexpr bool kPlatformMacOS = false;
#elif defined(__APPLE__)
inline constexpr const char* kPlatformName = "macOS";
inline constexpr bool kPlatformWindows = false;
inline constexpr bool kPlatformLinux = false;
inline constexpr bool kPlatformMacOS = true;
#else
inline constexpr const char* kPlatformName = "Unknown";
inline constexpr bool kPlatformWindows = false;
inline constexpr bool kPlatformLinux = false;
inline constexpr bool kPlatformMacOS = false;
#endif

// ── Architecture ─────────────────────────────────────────────

#if defined(__x86_64__) || defined(_M_X64)
inline constexpr const char* kArchitecture = "x86_64";
#elif defined(__i386__) || defined(_M_IX86)
inline constexpr const char* kArchitecture = "x86";
#elif defined(__aarch64__) || defined(_M_ARM64)
inline constexpr const char* kArchitecture = "ARM64";
#elif defined(__arm__) || defined(_M_ARM)
inline constexpr const char* kArchitecture = "ARM";
#else
inline constexpr const char* kArchitecture = "Unknown";
#endif

// ── C++ Standard ─────────────────────────────────────────────

inline constexpr int kCppStandard = __cplusplus;

} // namespace build
} // namespace quartz
