#pragma once

#include <functional>
#include <cstdint>

namespace logger::ctx {
    using Task = std::function<void()>;
    using TaskRunnerTag = uint64_t;
    using RepeatedTaskId = uint64_t;
}