#pragma once

#include <cstdint>
#include <string>

namespace quartz {
namespace file {

bool exists(const std::string& path) noexcept;
std::uint64_t size(const std::string& path) noexcept;
bool remove(const std::string& path) noexcept;
bool rename(const std::string& oldPath, const std::string& newPath) noexcept;
bool createDirectory(const std::string& path) noexcept;

} // namespace file
} // namespace quartz
