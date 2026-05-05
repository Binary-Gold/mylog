#pragma once

#include <memory>

#include "logger/context/executor.hpp"
#include "logger/context/using.hpp"

namespace logger::ctx {
    class Context {
    public:
        ~Context();

        Context(const Context&) = delete;
        Context& operator=(const Context&) = delete;

        Executor* GetExecutor();

        static Context* GetInstance() {
            static Context* initstate = new Context();
            return initstate;
        }

        TaskRunnerTag NewTaskRunner(TaskRunnerTag tag);

    private:
        Context();

        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}
