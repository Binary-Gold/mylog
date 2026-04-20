#include <memory>

#include "logging/sink.hpp"
#include "logging/formatter.hpp"
#include "logging/using.hpp"

namespace logger{
    struct ConsoleSink::Imp {
        std::unique_ptr<ForMatter> formatter_;
    };

    ConsoleSink::ConsoleSink() : imp_(std::make_unique<Imp>()) {}
    ConsoleSink::~ConsoleSink() = default;

    void ConsoleSink::Log(const LogMsg& msg) {
        MemoryBuf buf;
        imp_->formatter_->Fromat(msg, &buf);
        fwrite(buf.data(), 1, buf.size(), stdout);
        fwrite("\n", 1, 1, stdout);
    }
    void ConsoleSink::SetForMatter(std::unique_ptr<ForMatter> formatter) {
        imp_->formatter_ = std::move(formatter);
    }
}