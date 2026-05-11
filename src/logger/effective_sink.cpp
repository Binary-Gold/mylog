#include <atomic>
#include <cstring>
#include <fstream>
#include <iostream>

#include "logger/effective_sink.hpp"
#include "logger/compress/zstd_compress.hpp"
#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/proto/effective_formatter.hpp"
#include "logger/mmap/mmap.hpp"
#include "logger/utils/linux_ids.hpp"
#include "logger/context/entry.hpp"
#include "logger/utils/linux_ids.hpp"
#include "logger/utils/filestream_linux.hpp"

namespace logger {

struct EffectiveSink::Imp {
    Config config_;
    ctx::TaskRunnerTag task_runner_tag_;
    std::unique_ptr<EffectiveFormatter> formatter_;
    std::unique_ptr<compress::ZstdCompress> compress_;
    std::unique_ptr<crypt::AESCrypt> crypt_;
    std::unique_ptr<mmap::MMapAux> master_;
    std::unique_ptr<mmap::MMapAux> slave_;
    std::atomic<bool> slave_free_{true};
    std::filesystem::path log_file_path_;
};

EffectiveSink::EffectiveSink(const Config& config) : imp_(std::make_unique<Imp>()) {
    imp_->config_ = std::move(config);

    if (!std::filesystem::exists(imp_->config_.dir)) {
        std::filesystem::create_directories(imp_->config_.dir);
    }
    imp_->task_runner_tag_ = NEW_TASK_RUNNER(0);
    imp_->formatter_ = std::make_unique<EffectiveFormatter>();
    imp_->compress_ = std::make_unique<compress::ZstdCompress>();
    
    auto [my_pri, my_pub] = crypt::GenECDHKey();
    auto peer_pub = crypt::HexKeyToBinary(imp_->config_.pub_key);
    auto shared = crypt::GenECDHSharedSecret(my_pri, peer_pub);
    imp_->crypt_ =  std::make_unique<crypt::AESCrypt>(shared);

    imp_->master_ = std::make_unique<mmap::MMapAux>(imp_->config_.dir / "master_cache");
    imp_->slave_ = std::make_unique<mmap::MMapAux>(imp_->config_.dir / "slave_cache");

    if (!imp_->slave_->Empty()) {
        imp_->slave_free_.store(false);

    }

}

EffectiveSink::~EffectiveSink() = default;

void EffectiveSink::PrepareToFile_() {
  POST_TASK(imp_->task_runner_tag_, [this]() { CacheToFile_(); });
}

void EffectiveSink::CacheToFile_() {
    if (imp_->slave_free_.load()) {
        return;
    }

    if (imp_->slave_->Empty()) {
        imp_->slave_free_.store(true);
        return;
    }

    ChunkHeader chunk_header;
    chunk_header.size = imp_->slave_->Size();
    memcpy(chunk_header.pub_key, imp_->config_.pub_key.data(), imp_->config_.pub_key.size());
    // 写入块头与缓存内容
    auto file_path = GetFilePath_();
    std::ofstream ofs(file_path, std::ios::binary | std::ios::app);
    ofs.write(reinterpret_cast<char*>(&chunk_header), sizeof(chunk_header));
    ofs.write(reinterpret_cast<char*>(imp_->slave_->Data()), chunk_header.size);
    ofs.close();

    imp_->slave_->Clear();
    imp_->slave_free_.store(true);
}

std::filesystem::path EffectiveSink::GetFilePath_() {
  // {prefix}_{datetime}.log or {prefix}_{datetime}_{index}.log
  auto GetDateTimePath = [this]() -> std::filesystem::path {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
    if (!logger::utils::LocalCalendarTime(&now, &tm)) {
        tm = {};
    }
    char time_buf[32] = {0};
    std::strftime(time_buf, sizeof(time_buf), "%Y%m%d%H%M%S", &tm);
    return (imp_->config_.dir / (imp_->config_.prefix + "_" + time_buf));
  };

  if (imp_->log_file_path_.empty()) {
    imp_->log_file_ = std::make_unique<fs::Fd>(GetDateTimePath().string() + ".log");
  } else {
    bytes single_bytes = space_cast<bytes>(imp_->config_.single_size);
    if (imp_->log_file_->GetFileSize() > single_bytes.count()) {
      auto file_path = GetDateTimePath().string() + ".log";
      if (std::filesystem::exists(file_path)) {
        // 重名就加下标
        int index = 0;
        for (auto& p : std::filesystem::directory_iterator(imp_->config_.dir)) {
          if (p.path().filename().string().find(date_time_path.string()) != std::string::npos) {
            ++index;
          }
        }
        log_file_ = date_time_path.string() + "_" + std::to_string(index) + ".log";
      } else {
        log_file_ = file_path;
      }
    }
  }

  return log_file_;
}

}
