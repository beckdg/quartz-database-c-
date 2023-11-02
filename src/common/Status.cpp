#include "quartz/common/Status.h"

#include <sstream>

namespace quartz {

std::string Status::toString() const {
    if (ok()) {
        return "OK";
    }

    const char* codeStr = nullptr;
    switch (code_) {
        case Code::InvalidArgument: codeStr = "InvalidArgument"; break;
        case Code::IOError:         codeStr = "IOError";         break;
        case Code::Corruption:      codeStr = "Corruption";      break;
        case Code::OutOfMemory:     codeStr = "OutOfMemory";     break;
        case Code::Unknown:         codeStr = "Unknown";         break;
        default:                    codeStr = "Unknown";         break;
    }

    std::ostringstream oss;
    oss << codeStr << ": " << message_;
    return oss.str();
}

} // namespace quartz
