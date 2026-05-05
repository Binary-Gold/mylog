#include <chrono>
#include <cstdint>
#include <string>

#include "effective_msg.pb.h"
#include "logger/utils/linux_ids.hpp"
#include "logger/proto/effective_formatter.hpp"

namespace logger {
void EffectiveFormatter::Fromat(const LogMsg& msg, MemoryBuf* dest) {
    EffectiveMsg effective_msg;
    effective_msg.set_level(static_cast<std::int32_t>(msg.level));
    const auto now = std::chrono::system_clock::now();
    effective_msg.set_timestamp(
        std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch())
            .count());
    effective_msg.set_pid(logger::utils::GetProcessId());
    effective_msg.set_tid(logger::utils::GetThreadId());
    effective_msg.set_line(msg.location.line);
    effective_msg.set_file_name(std::string(msg.location.file_name));
    effective_msg.set_func_name(std::string(msg.location.func_name));
    effective_msg.set_log_info(std::string(msg.message));

    const std::size_t len = static_cast<std::size_t>(effective_msg.ByteSizeLong());
    dest->resize(len);
    (void)effective_msg.SerializeToArray(dest->data(), static_cast<int>(len));
}
} // namespace logger
