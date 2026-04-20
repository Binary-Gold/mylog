#pragma once

#include <functional>
#include <cstdint>

namespace context {
    using Task = std::function<void()>;
    using TaskRunnerTag = uint64_t;
    using RepeatedTaskId = uint64_t;
}