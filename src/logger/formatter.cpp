#include <ctime>
#include <string>

#include "logger/formatter.hpp"
#include "logger/logger_config.hpp"
#include "logger/utils/linux_ids.hpp"

namespace logger {
    void DefaultFormatter::Fromat(const LogMsg& msg, MemoryBuf* dest) {
        constexpr char kLogLevelStr[] = "TDIWEF";
        std::time_t now = std::time(nullptr);
        std::tm tm{};
        if (!logger::utils::LocalCalendarTime(&now, &tm)) {
            tm = {};
        }
        char time_buf[32] = {0};
        std::size_t time_len =
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm);
        dest->append("[", 1);
        dest->append(time_buf, time_len);
        dest->append("] [", 3);
        dest->append(1, kLogLevelStr[static_cast<int>(msg.level)]);
        dest->append("] [", 3);
        dest->append(msg.location.file_name.data(), msg.location.file_name.size());
        dest->append(":", 1);
        dest->append(std::to_string(msg.location.line));
        dest->append("] [", 3);
        dest->append(std::to_string(logger::utils::GetProcessId()));
        dest->append(":", 1);
        dest->append(std::to_string(logger::utils::GetThreadId()));
        dest->append("] ", 2);
        dest->append(msg.message.data(), msg.message.size());
    }
}