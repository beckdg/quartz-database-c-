#include "quartz/common/Version.h"

#include <sstream>

namespace quartz {

std::string Version::string() {
    return std::to_string(Major) + "." +
           std::to_string(Minor) + "." +
           std::to_string(Patch);
}

std::string Version::buildInfo() {
    std::ostringstream oss;
    oss << "QuartzDB " << string() << " "
        << __DATE__ << " " << __TIME__ << " "
        << "(C++ " << __cplusplus << ")";
    return oss.str();
}

} // namespace quartz
