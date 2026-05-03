#ifndef UTILS_LINUX_IDS_HPP
#define UTILS_LINUX_IDS_HPP

#include <cstdint>
#include <ctime>

namespace utils {

// Linux：当前进程 PID（getpid）
std::int32_t GetProcessId();

// Linux：当前线程 TID（内核线程号，syscall(SYS_gettid)）
std::int32_t GetThreadId();

// Linux/POSIX：UTC 秒级时间戳 → 本地日历时间（localtime_r）。成功返回 true。
bool LocalCalendarTime(std::time_t utc_seconds, std::tm* out);

} // namespace utils

#endif
