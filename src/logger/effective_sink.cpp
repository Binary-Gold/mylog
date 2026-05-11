#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <mutex>

#include "logger/effective_sink.hpp"
#include "logger/compress/zstd_compress.hpp"
#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/formatter.hpp"
#include "logger/proto/effective_formatter.hpp"
#include "logger/mmap/mmap.hpp"
#include "logger/utils/linux_ids.hpp"
#include "logger/context/entry.hpp"
#include "logger/utils/linux_ids.hpp"
#include "logger/utils/filestream_linux.hpp"
#include "logger/internal_log.hpp"
namespace logger {

struct EffectiveSink::Imp {
    Config config_;
    ctx::TaskRunnerTag task_runner_tag_ = NEW_TASK_RUNNER(0);
    std::unique_ptr<ForMatter> formatter_ = std::make_unique<EffectiveFormatter>();
    std::unique_ptr<compress::ZstdCompress> compress_ = std::make_unique<compress::ZstdCompress>();
    std::unique_ptr<crypt::AESCrypt> crypt_;
    std::unique_ptr<mmap::MMapAux> master_;
    std::unique_ptr<mmap::MMapAux> slave_;
    std::atomic<bool> slave_free_{true};
    std::filesystem::path log_file_path_;
    std::mutex mtx_;
    std::string compressed_buf_;
    std::string encryped_buf_;
};

EffectiveSink::EffectiveSink(const Config& config) : imp_(std::make_unique<Imp>()) {
    imp_->config_ = std::move(config);

    if (!std::filesystem::exists(imp_->config_.dir)) {
        std::filesystem::create_directories(imp_->config_.dir);
    }
    
    auto [my_pri, my_pub] = crypt::GenECDHKey();
    auto peer_pub = crypt::HexKeyToBinary(imp_->config_.pub_key);
    auto shared = crypt::GenECDHSharedSecret(my_pri, peer_pub);
    imp_->crypt_ =  std::make_unique<crypt::AESCrypt>(shared);

    imp_->master_ = std::make_unique<mmap::MMapAux>(imp_->config_.dir / "master_cache");
    imp_->slave_ = std::make_unique<mmap::MMapAux>(imp_->config_.dir / "slave_cache");

    if (!imp_->slave_->Empty()) {
        imp_->slave_free_.store(false);
        PrepareToFile_();
        WAIT_TASK_IDLE(imp_->task_runner_tag_);
    }

    if (!imp_->master_->Empty()) {
      if (imp_->slave_free_.load()) {
        imp_->slave_free_.store(false);
        SwapCache_();
      }
      PrepareToFile_();
    }
    POST_REPEATED_TASK(imp_->task_runner_tag_, [this]() { ElimateFiles_(); }, imp_->config_.interval, -1);
}

EffectiveSink::~EffectiveSink() = default;

void EffectiveSink::Log(const LogMsg& msg) {
  static thread_local MemoryBuf buf;
  imp_->formatter_->Fromat(msg, &buf);

  {
    std::lock_guard<std::mutex> lock(imp_->mtx_);
    imp_->compressed_buf_.resize(imp_->compress_->CompressedBound(buf.size()));
    size_t compressed_size = imp_->compress_->Compress(buf.data(), buf.size(), imp_->compressed_buf_.data(), imp_->compressed_buf_.size());
    if (compressed_size == 0) {
      LOG_ERROR("EffectiveSink::Log: compress failed");
      return;
    }
    imp_->encryped_buf_.clear();
    imp_->encryped_buf_.reserve(compressed_size + 16);
    imp_->crypt_->Encrypt(imp_->compressed_buf_.data(), compressed_size, imp_->encryped_buf_);
    if (imp_->encryped_buf_.empty()) {
      LOG_ERROR("EffectiveSink::Log: encrypt failed");
      return;
    }
    WriteToCache_(imp_->encryped_buf_.data(), imp_->encryped_buf_.size());
  }

  if (NeedCacheToFile_()) {
    if (imp_->slave_free_.load()) {
      imp_->slave_free_.store(false);
      SwapCache_();
    }
    PrepareToFile_();
  }
}

void EffectiveSink::PrepareToFile_() {
  POST_TASK(imp_->task_runner_tag_, [this]() { CacheToFile_(); });
}

void EffectiveSink::SetForMatter(std::unique_ptr<ForMatter> formatter) {
  imp_->formatter_ = std::move(formatter);
}

void EffectiveSink::Flush() {
  // 强制slave落盘
  PrepareToFile_();
  WAIT_TASK_IDLE(imp_->task_runner_tag_);

  // master落盘
  if (imp_->slave_free_.load()) {
    imp_->slave_free_.store(false);
    SwapCache_();
  }
  PrepareToFile_();
  WAIT_TASK_IDLE(imp_->task_runner_tag_);
}

bool EffectiveSink::NeedCacheToFile_() {
  return imp_->master_->Ratio() > 0.8;
}

void EffectiveSink::WriteToCache_(const void* data, uint32_t size) {
  ItemHeader item_header;
  item_header.size = size;
  imp_->master_->Push(&item_header, sizeof(item_header));
  imp_->master_->Push(data, size);
}

void EffectiveSink::SwapCache_() {
  std::lock_guard<std::mutex> lock(imp_->mtx_);
  std::swap(imp_->master_, imp_->slave_);
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
    auto file_path = UpdateFilePath_();
    std::ofstream ofs(file_path, std::ios::binary | std::ios::app);
    ofs.write(reinterpret_cast<char*>(&chunk_header), sizeof(chunk_header));
    ofs.write(reinterpret_cast<char*>(imp_->slave_->Data()), chunk_header.size);
    ofs.close();


    imp_->slave_->Clear();
    imp_->slave_free_.store(true);
}

std::filesystem::path EffectiveSink::UpdateFilePath_() {
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
    imp_->log_file_path_ = GetDateTimePath().string() + ".log";
  } else {
    auto file_size = fs::GetFileSize(imp_->log_file_path_);
    bytes single_bytes = space_cast<bytes>(imp_->config_.single_size);
    if (file_size > single_bytes.count()) {
      auto date_time_path = GetDateTimePath();
      auto file_path = date_time_path.string() + ".log";
      if (std::filesystem::exists(file_path)) {
        // 重名就加下标
        std::filesystem::path candidate = file_path;
        int index = 0;
        while (std::filesystem::exists(candidate)) {
            ++index;
        }
        imp_->log_file_path_ = date_time_path.string() + "_" + std::to_string(index) + ".log";
      } else {
        imp_->log_file_path_ = file_path;
      }
    }
  }

  return imp_->log_file_path_;
}

void EffectiveSink::ElimateFiles_() {
  LOG_INFO("EffectiveSink::ElimateFiles_: start");
  std::vector<std::filesystem::path> files;
  for (auto& p : std::filesystem::directory_iterator(imp_->config_.dir)) {
    if (p.path().extension() == ".log") {
      files.push_back(p.path());
    }
  }

  std::sort(files.begin(), files.end(), [](const std::filesystem::path& lhs, const std::filesystem::path& rhs) {
    return std::filesystem::last_write_time(lhs) > std::filesystem::last_write_time(rhs);
  });

  size_t total_bytes = space_cast<bytes>(imp_->config_.total_size).count();
  size_t used_bytes = 0;
  for (auto& file : files) {
    used_bytes += fs::GetFileSize(file);
    if (used_bytes > total_bytes) {
      LOG_INFO("EffectiveSink::ElimateFiles_: remove file={}", file.string());
      std::filesystem::remove(file);
    }
  }
}

}
