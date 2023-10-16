#pragma once

#include <cstdint>

namespace quartz {
namespace space {

struct AllocationHints {
    bool preferContiguous = false;
    bool preferBeginning = false;
    bool preferEnd = false;
    std::uint32_t minExtentLength = 1;
    bool exactFit = false;
};

} // namespace space
} // namespace quartz
