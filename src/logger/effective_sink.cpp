#if 0

#include <atomic>

#include "logger/effective_sink.hpp"
#include "logger/compress/zlib_compress.hpp"
#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/proto/effective_formatter.hpp"
#include "logger/mmap/mmap.hpp"
#include "logger/utils/linux_ids.hpp"

namespace logger {

struct EffectiveSink::Imp {
    EffectiveSink::Config config_;
    logger::ctx::TaskRunnerTag task_runner_{};
    std::unique_ptr<EffectiveFormatter> formatter_;

    std::string client_pub_key_;
    std::unique_ptr<logger::crypt::Logcrypt> crypt_;
    std::unique_ptr<logger::compress::Compression> compress_;

    std::unique_ptr<logger::mmap::MMapAux> master_cache_;
    std::unique_ptr<logger::mmap::MMapAux> slave_cache_;
    std::atomic<bool> is_slave_free_{false};

};

EffectiveSink::EffectiveSink(const Config& config) : imp_(std::make_unique<Imp>()) {
    imp_->config_ = std::move(config);

    LOG_INFO("EffectiveSink: dir={}, prefix={}, pub_key={}, interval={}, single_size={}, total_size={}",
        imp_->config_.dir.string(), imp_->config_.prefix, imp_->config_.pub_key, imp_->config_.interval.count(),
        imp_->config_.single_size.count(), imp_->config_.total_size.count());

    if (!std::filesystem::exists(imp_->config_.dir)) {
        std::filesystem::create_directories(imp_->config_.dir);
    }

    imp_->task_runner_ = NEW_TASK_RUNNER(6666);
    imp_->formatter_ = std::make_unique<EffectiveFormatter>();

    auto ecdh_key = logger::crypt::GenECDHKey();
    auto client_pri = std::get<0>(ecdh_key);
    imp_->client_pub_key_ = std::get<1>(ecdh_key);
    LOG_INFO("EffectiveSink: client pub size {}", imp_->client_pub_key_.size());
    std::string svr_pub_key_bin = logger::crypt::HexKeyToBinary(imp_->config_.pub_key);
    std::string shared_secret = logger::crypt::GenECDHSharedSecret(client_pri, svr_pub_key_bin);
    imp_->crypt_ = std::make_unique<logger::crypt::AESCrypt>(shared_secret);

    imp_->compress_ = std::make_unique<logger::compress::ZlibCompress>();

    imp_->master_cache_ = std::make_unique<MMapAux>(imp_->config_.dir / "master_cache");
    imp_->slave_cache_ = std::make_unique<MMapAux>(imp_->config_.dir / "slave_cache");
    if (!imp_->master_cache_ || !imp_->slave_cache_) {
        throw std::runtime_error("EffectiveSink::EffectiveSink: create mmap failed");
    }
    if (!imp_->slave_cache_->Empty()) {
        imp_->is_slave_free_.store(false);
        PrepareToFile_();
        WAIT_TASK_IDLE(imp_->task_runner_);
    }
    if (!imp_->master_cache->Empty()) {
        if (imp_->is_slave_free_.load()) {
            imp_->is_slave_free_.store(false);
            SwapCache_();
        }
        PrepareToFile_();
    }
}

EffectiveSink::~EffectiveSink() = default;

void EffectiveSink::Log(const LogMsg&) {}

void EffectiveSink::SetForMatter(std::unique_ptr<ForMatter> formatter) {
    (void)formatter;
}

void EffectiveSink::Flush() {}
} // namespace logger

void EffectiveSink::SwapCache_() {

}

void EffectiveSink::PrepareToFile_() {
  POST_TASK(imp_->task_runner_, [this]() { CacheToFile_(); });
}

void EffectiveSink::CacheToFile_() {
    if (imp_->is_slave_free_.load()) {
        return;
    }
    if (imp_->slave_cache_->Empty()) {
        imp_->is_slave_free_.store(true);
        return;
    }

}

#endif
