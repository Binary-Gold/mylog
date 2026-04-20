#include <ctime>
#include <unistd.h> 
#include <thread>

#include "logging/formatter.hpp"
#include "logging/logger_config.hpp"
// todo 不跨平台了

namespace logger {
    void DefaultFormatter::Fromat(const LogMsg& msg, MemoryBuf* dest) {
        constexpr char kLogLevelStr[] = "TDIWEF";
        std::time_t now = std::time(nullptr);
        std::tm tm;
        localtime_r(&now, &tm);
        char time_buf[32] = {0};
        std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", &tm);
        dest->append("[", 1);
        dest->append(time_buf, sizeof(time_buf));
        dest->append("] [", 3);
        dest->append(1, kLogLevelStr[static_cast<int>(msg.level)]);
        dest->append("] [", 3);
        dest->append(msg.location.file_name.data(), msg.location.file_name.size());
        dest->append(":", 1);
        dest->append(std::to_string(msg.location.line));
        dest->append("] [", 3);
        dest->append(std::to_string(getpid()));
        dest->append(":", 1);
        dest->append(std::to_string(std::hash<std::thread::id>{}((std::this_thread::get_id()))));
        dest->append("] ", 2);
        dest->append(msg.message.data(), msg.message.size());
    }
}