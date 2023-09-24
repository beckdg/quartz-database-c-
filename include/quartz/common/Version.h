#pragma once

#include <string>

namespace quartz {

struct Version {
    static constexpr int Major = 1;
    static constexpr int Minor = 0;
    static constexpr int Patch = 0;

    Version() = delete;

    static std::string string();
    static std::string buildInfo();
};

} // namespace quartz
