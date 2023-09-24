#pragma once

#include <utility>

namespace quartz {

template <typename Callable>
class ScopeGuard {
public:
    explicit ScopeGuard(Callable func) noexcept(std::is_nothrow_move_constructible_v<Callable>)
        : func_(std::move(func)), active_(true) {}

    ~ScopeGuard() noexcept(noexcept(func_())) {
        if (active_) {
            func_();
        }
    }

    void dismiss() noexcept { active_ = false; }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;
    ScopeGuard(ScopeGuard&&) = delete;
    ScopeGuard& operator=(ScopeGuard&&) = delete;

private:
    Callable func_;
    bool active_;
};

template <typename Callable>
auto makeScopeGuard(Callable&& func) -> ScopeGuard<std::decay_t<Callable>> {
    return ScopeGuard<std::decay_t<Callable>>(std::forward<Callable>(func));
}

} // namespace quartz

#define QUARTZ_CONCAT_IMPL(a, b) a##b
#define QUARTZ_CONCAT(a, b) QUARTZ_CONCAT_IMPL(a, b)

#define QUARTZ_SCOPE_EXIT \
    auto QUARTZ_CONCAT(quartz_scope_guard_, __LINE__) = ::quartz::makeScopeGuard
