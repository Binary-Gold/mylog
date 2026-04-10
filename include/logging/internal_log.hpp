#pragma once

#ifdef ENABLE_LOG

#include "third_party/fmt/include/fmt/chrono.h"
#include "third_party/fmt/include/fmt/core.h"

#define LOG_INFO(...) \
    fmt::print("[INFO] [{} {}] {} {}", __FILE__, __LINE__, std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))
#define LOG_WARN(...) \
    fmt::print("[WARN] [{} {}] {} {}", __FILE__, __LINE__, std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))
#define LOG_ERROR(...) \
    fmt::print("[ERROR] [{} {}] {} {}", __FILE__, __LINE__, std::chrono::system_clock::now(), fmt::format(__VA_ARGS__))

#else

#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)

#endif
