#include "quartz/storage/DatabaseFile.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

namespace quartz {
namespace storage {

DatabaseFile::DatabaseFile(const std::string& path, bool create) {
    (void)open(path, create);
}

DatabaseFile::~DatabaseFile() noexcept {
    if (stream_.is_open()) {
        (void)close();
    }
}

DatabaseFile::DatabaseFile(DatabaseFile&& other) noexcept
    : path_(std::move(other.path_))
    , stream_(std::move(other.stream_))
    , size_(other.size_) {

    other.size_ = 0;
}

DatabaseFile& DatabaseFile::operator=(DatabaseFile&& other) noexcept {
    if (this != &other) {
        if (stream_.is_open()) {
            (void)close();
        }

        path_ = std::move(other.path_);
        stream_ = std::move(other.stream_);
        size_ = other.size_;

        other.size_ = 0;
    }
    return *this;
}

Status DatabaseFile::open(const std::string& path, bool create) {
    if (stream_.is_open()) {
        return Status::ioError("File already open: " + path_);
    }

    path_ = path;

    auto fileExists = fs::exists(fs::path(path));

    if (create && !fileExists) {
        stream_.open(path, std::ios::binary | std::ios::in | std::ios::out | std::ios::trunc);
        if (!stream_.is_open()) {
            return Status::ioError("Failed to create file: " + path);
        }
        size_ = 0;
        return Status::success();
    }

    stream_.open(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!stream_.is_open()) {
        return Status::ioError("Failed to open file: " + path);
    }

    stream_.seekg(0, std::ios::end);
    size_ = static_cast<std::uint64_t>(stream_.tellg());
    stream_.seekg(0, std::ios::beg);

    return Status::success();
}

Status DatabaseFile::close() noexcept {
    if (!stream_.is_open()) {
        return Status::success();
    }

    stream_.close();
    path_.clear();
    size_ = 0;

    if (stream_.is_open()) {
        return Status::ioError("Failed to close file");
    }
    return Status::success();
}

Status DatabaseFile::flush() {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }
    stream_.flush();
    if (stream_.fail()) {
        return Status::ioError("Failed to flush file");
    }
    return Status::success();
}

Status DatabaseFile::sync() {
    return flush();
}

std::uint64_t DatabaseFile::fileSize() const noexcept {
    return size_;
}

Status DatabaseFile::seek(std::uint64_t position) {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }
    stream_.seekg(static_cast<std::streamoff>(position), std::ios::beg);
    stream_.seekp(static_cast<std::streamoff>(position), std::ios::beg);
    if (stream_.fail()) {
        return Status::ioError("Failed to seek in file");
    }
    return Status::success();
}

Status DatabaseFile::readBytes(void* buffer, std::size_t count) {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }
    stream_.read(static_cast<char*>(buffer), static_cast<std::streamsize>(count));
    if (stream_.fail() && !stream_.eof()) {
        return Status::ioError("Failed to read from file");
    }
    return Status::success();
}

Status DatabaseFile::writeBytes(const void* buffer, std::size_t count) {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }
    stream_.write(static_cast<const char*>(buffer), static_cast<std::streamsize>(count));
    if (stream_.fail()) {
        return Status::ioError("Failed to write to file");
    }

    auto pos = static_cast<std::uint64_t>(stream_.tellp());
    if (pos > size_) {
        size_ = pos;
    }
    return Status::success();
}

Status DatabaseFile::resize(std::uint64_t newSize) {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }

    stream_.close();

    try {
        fs::resize_file(fs::path(path_), static_cast<std::uintmax_t>(newSize));
        size_ = newSize;
        stream_.open(path_, std::ios::binary | std::ios::in | std::ios::out);
        if (!stream_.is_open()) {
            return Status::ioError("Failed to reopen file after resize");
        }
        return Status::success();
    } catch (const fs::filesystem_error& e) {
        stream_.open(path_, std::ios::binary | std::ios::in | std::ios::out);
        return Status::ioError(std::string("Failed to resize file: ") + e.what());
    }
}

Status DatabaseFile::readPage(Page& page) {
    if (page.id() == kInvalidPageId) {
        return Status::invalidArgument("Cannot read page with invalid ID");
    }
    auto pos = static_cast<std::uint64_t>(page.id()) * kPageSize;
    auto result = seek(pos);
    if (!result.ok()) {
        return result;
    }

    PageHeader header;
    result = readBytes(&header, kPageHeaderSize);
    if (!result.ok()) {
        return result;
    }

    if (!header.isValid()) {
        return Status::corruption("Invalid page header at page " +
                                  std::to_string(page.id()));
    }

    page = Page(page.id(), header.pageType);
    page.setGeneration(header.generation);

    result = readBytes(page.payload(), kPagePayloadSize);
    if (!result.ok()) {
        return result;
    }

    page.setDirty(false);
    return Status::success();
}

Status DatabaseFile::writePage(const Page& page) {
    if (page.id() == kInvalidPageId) {
        return Status::invalidArgument("Cannot write page with invalid ID");
    }
    auto pos = static_cast<std::uint64_t>(page.id()) * kPageSize;
    auto result = seek(pos);
    if (!result.ok()) {
        return result;
    }

    result = writeBytes(&page.header(), kPageHeaderSize);
    if (!result.ok()) {
        return result;
    }

    result = writeBytes(page.payload(), kPagePayloadSize);
    if (!result.ok()) {
        return result;
    }

    return Status::success();
}

Status DatabaseFile::ensureOpen() const noexcept {
    if (!stream_.is_open()) {
        return Status::ioError("File not open");
    }
    return Status::success();
}

} // namespace storage
} // namespace quartz
