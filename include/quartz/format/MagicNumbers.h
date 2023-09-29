#pragma once

#include <cstdint>

namespace quartz {
namespace format {

struct MagicNumbers {
    static constexpr std::uint32_t kDatabaseMagic   = 0x51444231u; // "QDB1"
    static constexpr std::uint32_t kSuperblockMagic = 0x51444253u; // "QDBS"
    static constexpr std::uint32_t kPageMagic       = 0x50475132u; // "PGQ2"
    static constexpr std::uint32_t kFreeListMagic   = 0x464C5133u; // "FLQ3"
    static constexpr std::uint32_t kMetadataMagic   = 0x4D445134u; // "MDQ4"
    static constexpr std::uint32_t kJournalMagic    = 0x4A524E35u; // "JRN5"

    static constexpr std::uint32_t kFormatMagicV1   = 0x51444231u; // "QDB1"
    static constexpr std::uint32_t kFormatMagicV2   = 0x51444232u; // "QDB2"

    static bool isValid(std::uint32_t value) noexcept {
        return value == kFormatMagicV1 || value == kFormatMagicV2;
    }

    static bool isRecognized(std::uint32_t value) noexcept {
        return value == kDatabaseMagic || value == kSuperblockMagic ||
               value == kPageMagic || value == kFreeListMagic ||
               value == kMetadataMagic || value == kJournalMagic;
    }
};

} // namespace format
} // namespace quartz
