#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/storage/DatabaseFile.h"
#include "quartz/serialization/BufferView.h"
#include "quartz/wal/LogRecord.h"

#include <cstdint>
#include <string>

namespace quartz {
namespace wal {

#pragma pack(push, 1)
struct WalFileHeader {
    std::uint32_t magic = 0;
    std::uint32_t version = 0;
    std::uint32_t pageSize = 0;
    std::uint32_t reserved = 0;

    static constexpr std::size_t kSize = 16;
};
#pragma pack(pop)

static_assert(sizeof(WalFileHeader) == WalFileHeader::kSize, "WalFileHeader size mismatch");

/// Append-only WAL file backed by DatabaseFile.
class LogFile : private NonCopyable {
public:
    LogFile();
    explicit LogFile(storage::DatabaseFile& file);

    Status create(const std::string& path);
    Status open(const std::string& path, bool createIfMissing = false);
    Status close();

    bool isOpen() const noexcept;
    const std::string& path() const noexcept;

    Status append(serialization::BufferView data);
    Status appendRecord(const LogRecord& record);
    Status readBytes(std::uint64_t offset, void* buffer, std::size_t count) const;
    Status readRecord(std::uint64_t offset, LogRecord& record, std::uint64_t& bytesRead) const;

    Status flush();
    Status truncate(std::uint64_t newSize);

    std::uint64_t fileSize() const noexcept;
    std::uint64_t dataOffset() const noexcept;
    std::uint64_t appendOffset() const noexcept;

    Status validate() const;

    storage::DatabaseFile& file() noexcept { return *file_; }
    const storage::DatabaseFile& file() const noexcept { return *file_; }

private:
    bool ownsFile_ = false;
    mutable storage::DatabaseFile ownedFile_;
    storage::DatabaseFile* file_ = nullptr;
    std::uint64_t appendOffset_ = WalFileHeader::kSize;

    Status writeHeader();
    Status readHeader(WalFileHeader& header) const;
    Status alignAppendOffset();
};

} // namespace wal
} // namespace quartz
