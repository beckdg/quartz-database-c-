#pragma once

#include "quartz/common/NonCopyable.h"
#include "quartz/common/Status.h"
#include "quartz/storage/Page.h"
#include "quartz/storage/StorageConstants.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>

namespace quartz {
namespace storage {

class DatabaseFile : private NonCopyable {
public:
    DatabaseFile() noexcept = default;
    explicit DatabaseFile(const std::string& path, bool create = false);
    ~DatabaseFile() noexcept;

    DatabaseFile(DatabaseFile&& other) noexcept;
    DatabaseFile& operator=(DatabaseFile&& other) noexcept;

    Status open(const std::string& path, bool create = false);
    Status close() noexcept;
    bool isOpen() const noexcept { return stream_.is_open(); }

    Status flush();
    Status sync();

    std::uint64_t fileSize() const noexcept;

    Status seek(std::uint64_t position);
    Status readBytes(void* buffer, std::size_t count);
    Status writeBytes(const void* buffer, std::size_t count);

    Status resize(std::uint64_t newSize);

    Status readPage(Page& page);
    Status writePage(const Page& page);

    const std::string& path() const noexcept { return path_; }

private:
    Status ensureOpen() const noexcept;

    std::string path_;
    mutable std::fstream stream_;
    std::uint64_t size_ = 0;
};

} // namespace storage
} // namespace quartz
