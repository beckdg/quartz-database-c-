#pragma once

namespace quartz {

class NonMovable {
protected:
    NonMovable() = default;
    ~NonMovable() = default;

    NonMovable(NonMovable&&) = delete;
    NonMovable& operator=(NonMovable&&) = delete;
};

} // namespace quartz
