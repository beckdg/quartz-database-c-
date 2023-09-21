#pragma once

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace config {

// ── Version ──────────────────────────────────────────────────
inline constexpr int kVersionMajor    = 1;
inline constexpr int kVersionMinor    = 0;
inline constexpr int kVersionPatch    = 0;

// ── File Format ──────────────────────────────────────────────
inline constexpr std::uint32_t kMagicNumber       = 0x51444231u;   // "QDB1"
inline constexpr std::uint32_t kFileFormatVersion  = 1;

// ── Page Constants ───────────────────────────────────────────
inline constexpr std::size_t kPageSize            = 4096;
inline constexpr std::size_t kPageHeaderSize      = 64;
inline constexpr std::size_t kPagePayloadSize     = kPageSize - kPageHeaderSize;

inline constexpr std::uint32_t kInvalidPageId     = 0xFFFFFFFFu;
inline constexpr std::uint32_t kMaxPageCount      = 1024 * 1024;
inline constexpr std::uint32_t kReservedPages     = 8;

// ── Alignment ────────────────────────────────────────────────
inline constexpr std::size_t kDefaultAlignment    = 8;
inline constexpr std::size_t kPageAlignment       = 4096;

// ── Checksum ─────────────────────────────────────────────────
inline constexpr std::size_t kChecksumSize       = 4;
inline constexpr std::size_t kChecksumFieldOffset = kPageHeaderSize - kChecksumSize;

// ── File ─────────────────────────────────────────────────────
inline constexpr const char* kFileExtension       = ".qdb";
inline constexpr std::size_t kInitialFileSize     = kPageSize * 8;   // 32 KB

} // namespace config
} // namespace quartz
