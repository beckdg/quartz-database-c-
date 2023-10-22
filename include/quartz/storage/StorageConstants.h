#pragma once

#include "quartz/common/Config.h"

#include <cstddef>
#include <cstdint>

namespace quartz {
namespace storage {

// ── Page ID Type ─────────────────────────────────────────────
using PageId = std::uint32_t;

inline constexpr PageId kInvalidPageId   = config::kInvalidPageId;
inline constexpr PageId kMaxPageId       = config::kMaxPageCount - 1;
inline constexpr PageId kHeaderPageId    = 0;
inline constexpr PageId kMetadataPageId  = 1;
inline constexpr PageId kFirstDataPageId = config::kReservedPages;

inline constexpr std::size_t kPageSize         = config::kPageSize;
inline constexpr std::size_t kPageHeaderSize   = config::kPageHeaderSize;
inline constexpr std::size_t kPagePayloadSize  = config::kPagePayloadSize;
inline constexpr std::size_t kReservedPageCount = config::kReservedPages;

inline constexpr std::size_t kDefaultAlignment = config::kDefaultAlignment;

} // namespace storage
} // namespace quartz
