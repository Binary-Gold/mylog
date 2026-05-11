#include "logger/utils/linux_ids.hpp"

#include <ctime>
#include <sys/syscall.h>
#include <unistd.h>

namespace logger::utils {

std::int32_t GetProcessId() {
    return static_cast<std::int32_t>(::getpid());
}

std::int32_t GetThreadId() {
    return static_cast<std::int32_t>(::syscall(SYS_gettid));
}

bool LocalCalendarTime(std::time_t* utc_seconds, std::tm* out) {
    if (!out) {
        return false;
    }
    return ::localtime_r(utc_seconds, out) != nullptr;
}

} // namespace logger::utils
