#include "quartz/util/FileUtils.h"

#include <filesystem>
#include <system_error>

namespace fs = std::filesystem;

namespace quartz {
namespace file {

bool exists(const std::string& path) noexcept {
    std::error_code ec;
    return fs::exists(fs::path(path), ec);
}

std::uint64_t size(const std::string& path) noexcept {
    std::error_code ec;
    auto p = fs::path(path);
    auto sz = fs::file_size(p, ec);
    if (ec) return 0;
    return static_cast<std::uint64_t>(sz);
}

bool remove(const std::string& path) noexcept {
    std::error_code ec;
    return fs::remove(fs::path(path), ec);
}

bool rename(const std::string& oldPath, const std::string& newPath) noexcept {
    std::error_code ec;
    fs::rename(fs::path(oldPath), fs::path(newPath), ec);
    return !ec;
}

bool createDirectory(const std::string& path) noexcept {
    std::error_code ec;
    return fs::create_directories(fs::path(path), ec);
}

} // namespace file
} // namespace quartz
