#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include "logger/compress/zstd_compress.hpp"
#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/effective_sink.hpp"
#include "effective_msg.pb.h"

using namespace logger;

namespace {

struct ParsedEntry {
    int level;
    std::string level_str;
    std::string timestamp;
    int pid;
    int tid;
    std::string file;
    int line;
    std::string func;
    std::string msg;
};

const char* LevelName(int lvl) {
    switch (static_cast<LogLevel>(lvl)) {
    case LogLevel::kTrace: return "TRACE";
    case LogLevel::kDebug: return "DEBUG";
    case LogLevel::kInfo:  return "INFO";
    case LogLevel::kWarn:  return "WARN";
    case LogLevel::kError: return "ERROR";
    case LogLevel::kFatal: return "FATAL";
    default:               return "?";
    }
}

std::string FormatTime(int64_t ms_since_epoch) {
    auto tp = std::chrono::system_clock::time_point(std::chrono::milliseconds(ms_since_epoch));
    auto t = std::chrono::system_clock::to_time_t(tp);
    auto ms = ms_since_epoch % 1000;

    std::tm tm{};
    localtime_r(&t, &tm);

    char buf[32];
    auto n = std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    snprintf(buf + n, sizeof(buf) - n, ".%03ld", ms);
    return buf;
}

ParsedEntry ParseOne(compress::ZstdCompress& zstd, crypt::AESCrypt& aes,
                      const void* data, uint32_t size) {
    // 解密
    auto decrypted = aes.Decrypt(data, size);
    // 解压
    auto decompressed = zstd.Decompress(decrypted.data(), decrypted.size());
    // 反序列化
    EffectiveMsg msg;
    msg.ParseFromArray(decompressed.data(), static_cast<int>(decompressed.size()));

    ParsedEntry e;
    e.level     = msg.level();
    e.level_str = LevelName(msg.level());
    e.timestamp = FormatTime(msg.timestamp());
    e.pid       = msg.pid();
    e.tid       = msg.tid();
    e.file      = msg.file_name();
    e.line      = msg.line();
    e.func      = msg.func_name();
    e.msg       = msg.log_info();
    return e;
}

void PrintEntry(const ParsedEntry& e, bool color) {
    if (color) {
        const char* c = "\033[0m";
        switch (static_cast<LogLevel>(e.level)) {
        case LogLevel::kWarn:  c = "\033[33m"; break;
        case LogLevel::kError:
        case LogLevel::kFatal: c = "\033[31m"; break;
        case LogLevel::kDebug: c = "\033[36m"; break;
        case LogLevel::kTrace: c = "\033[90m"; break;
        default: break;
        }
        std::cout << c;
    }

    std::cout << "[" << e.timestamp << "]"
              << " [" << e.level_str << "]"
              << " [" << e.pid << ":" << e.tid << "]"
              << " " << e.file << ":" << e.line
              << " " << e.func << " | " << e.msg;

    if (color) std::cout << "\033[0m";
    std::cout << '\n';
}

int ParseFile(const std::filesystem::path& log_path,
               const std::string& server_priv_hex) {
    std::ifstream ifs(log_path, std::ios::binary);
    if (!ifs) {
        std::cerr << "无法打开文件: " << log_path << '\n';
        return 1;
    }

    // 获取 ECDH 公钥长度 (secp256r1)
    auto [_, dummy_pub] = crypt::GenECDHKey();
    const size_t kPubKeyLen = dummy_pub.size();

    compress::ZstdCompress zstd;
    bool first_chunk = true;

    while (ifs.peek() != EOF) {
        ChunkHeader header;
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!ifs) break;

        if (header.magic != ChunkHeader::kMagic) {
            std::cerr << "ChunkHeader 魔数不匹配: 0x"
                      << std::hex << header.magic << '\n';
            return 1;
        }

        // 提取客户端公钥
        std::string client_pub(header.pub_key, kPubKeyLen);
        auto server_priv = crypt::HexKeyToBinary(server_priv_hex);

        // ECDH → 共享密钥 → AES
        auto shared = crypt::GenECDHSharedSecret(server_priv, client_pub);
        crypt::AESCrypt aes(shared);

        if (first_chunk) {
            first_chunk = false;
        }

        // 读取块数据
        std::vector<char> chunk_data(header.size);
        ifs.read(chunk_data.data(), header.size);
        if (!ifs) {
            std::cerr << "读取块数据失败\n";
            return 1;
        }

        // 逐条解析
        size_t offset = 0;
        while (offset + sizeof(ItemHeader) <= header.size) {
            ItemHeader item;
            std::memcpy(&item, chunk_data.data() + offset, sizeof(item));
            offset += sizeof(item);

            if (item.magic != ItemHeader::kMagic) {
                std::cerr << "ItemHeader 魔数不匹配 at offset=" << offset << '\n';
                break;
            }

            if (offset + item.size > header.size) {
                std::cerr << "Item 数据越界\n";
                break;
            }

            try {
                auto entry = ParseOne(zstd, aes, chunk_data.data() + offset, item.size);
                PrintEntry(entry, true);
            } catch (const std::exception& e) {
                std::cerr << "解析条目失败: " << e.what() << '\n';
            }

            offset += item.size;
        }
    }

    return 0;
}

void Usage(const char* prog) {
    std::cout << "用法:\n"
              << "  " << prog << " <log_file> <server_priv_key_hex>\n"
              << "示例:\n"
              << "  # 生成密钥对\n"
              << "  # 将公钥配置给 EffectiveSink, 私钥留给解析器\n"
              << "  " << prog << " test_20260511120000.log "
              << "\"a1b2c3d4e5f6...\"\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        Usage(argv[0]);
        return 1;
    }

    std::filesystem::path log_path(argv[1]);
    std::string server_priv_hex(argv[2]);

    if (!std::filesystem::exists(log_path)) {
        std::cerr << "文件不存在: " << log_path << '\n';
        return 1;
    }

    return ParseFile(log_path, server_priv_hex);
}
