#pragma once

#include <string>
#include <string_view>

namespace quartz {

class [[nodiscard]] Status {
public:
    enum class Code : int {
        Ok = 0,
        InvalidArgument = 1,
        IOError = 2,
        Corruption = 3,
        OutOfMemory = 4,
        Unknown = 99
    };

    static Status success() noexcept { return Status(); }
    static Status invalidArgument(std::string msg) { return Status(Code::InvalidArgument, std::move(msg)); }
    static Status ioError(std::string msg)         { return Status(Code::IOError, std::move(msg)); }
    static Status corruption(std::string msg)      { return Status(Code::Corruption, std::move(msg)); }
    static Status outOfMemory(std::string msg)     { return Status(Code::OutOfMemory, std::move(msg)); }
    static Status unknown(std::string msg)         { return Status(Code::Unknown, std::move(msg)); }

    Status() noexcept = default;
    Status(const Status&) = default;
    Status& operator=(const Status&) = default;
    Status(Status&&) noexcept = default;
    Status& operator=(Status&&) noexcept = default;

    bool ok() const noexcept { return code_ == Code::Ok; }
    Code code() const noexcept { return code_; }
    std::string_view message() const noexcept { return message_; }

    bool operator==(const Status& rhs) const noexcept {
        return code_ == rhs.code_ && message_ == rhs.message_;
    }
    bool operator!=(const Status& rhs) const noexcept { return !(*this == rhs); }

    explicit operator bool() const = delete;

    std::string toString() const;

private:
    Status(Code code, std::string msg)
        : code_(code), message_(std::move(msg)) {}

    Code code_ = Code::Ok;
    std::string message_;
};

} // namespace quartz
