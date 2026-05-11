#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>

#include "logger/compress/zstd_compress.hpp"
#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"
#include "logger/effective_sink.hpp"
#include "effective_msg.pb.h"

using namespace logger;

int main() {
    auto dir = std::filesystem::temp_directory_path() / "mylog_rt_test";
    std::filesystem::remove_all(dir);

    // 1. 生成服务端密钥对
    auto [server_pri, server_pub] = crypt::GenECDHKey();
    auto server_pri_hex = crypt::BinaryKeyToHex(server_pri);

    // 2. 创建 EffectiveSink
    EffectiveSink::Config cfg;
    cfg.dir        = dir;
    cfg.prefix     = "rt";
    cfg.pub_key    = crypt::BinaryKeyToHex(server_pub);
    cfg.single_size = megabytes(64);
    cfg.total_size  = megabytes(256);
    EffectiveSink sink(cfg);

    // 3. 写入日志
    std::vector<std::string> inputs = {
        "[INFO] hello round trip",
        "[DEBUG] x=42 y=hello",
        "[ERROR] something broken",
    };

    for (auto& msg : inputs) {
        LogMsg log_msg(LogLevel::kInfo, msg);
        sink.Log(log_msg);
    }
    sink.Flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 4. 找到日志文件
    std::filesystem::path log_file;
    for (auto& p : std::filesystem::directory_iterator(dir)) {
        if (p.path().extension() == ".log") {
            log_file = p.path();
            break;
        }
    }

    if (log_file.empty()) {
        std::cerr << "FAIL: 未生成日志文件\n";
        return 1;
    }
    std::cout << "日志文件: " << log_file << '\n';

    // 5. 解析
    std::ifstream ifs(log_file, std::ios::binary);

    auto [_, dummy_pub] = crypt::GenECDHKey();
    const size_t kPubKeyLen = dummy_pub.size();

    compress::ZstdCompress zstd;
    int count = 0;
    bool ok = true;

    while (ifs.peek() != EOF) {
        ChunkHeader header;
        ifs.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!ifs) break;

        if (header.magic != ChunkHeader::kMagic) {
            std::cerr << "FAIL: ChunkHeader 魔数错误\n";
            ok = false; break;
        }

        std::string client_pub(header.pub_key, kPubKeyLen);
        auto server_priv_bin = crypt::HexKeyToBinary(server_pri_hex);
        auto shared = crypt::GenECDHSharedSecret(server_priv_bin, client_pub);
        crypt::AESCrypt aes(shared);

        std::vector<char> chunk(header.size);
        ifs.read(chunk.data(), header.size);

        size_t off = 0;
        while (off + sizeof(ItemHeader) <= header.size) {
            ItemHeader item;
            std::memcpy(&item, chunk.data() + off, sizeof(item));
            off += sizeof(item);

            if (item.magic != ItemHeader::kMagic) {
                std::cerr << "FAIL: ItemHeader 魔数错误\n";
                ok = false; break;
            }

            try {
                auto decrypted  = aes.Decrypt(chunk.data() + off, item.size);
                auto decompressed = zstd.Decompress(decrypted.data(), decrypted.size());
                EffectiveMsg msg;
                msg.ParseFromArray(decompressed.data(), static_cast<int>(decompressed.size()));

                std::cout << "[" << count << "] " << msg.log_info() << '\n';
                ++count;
            } catch (const std::exception& e) {
                std::cerr << "FAIL: 解析条目失败: " << e.what() << '\n';
                ok = false;
            }
            off += item.size;
        }
    }

    if (ok && count == static_cast<int>(inputs.size())) {
        std::cout << "\nPASS: round-trip 成功, " << count << " 条日志全部恢复\n";
    } else {
        std::cout << "\nFAIL: 期望 " << inputs.size() << " 条, 实际 " << count << " 条\n";
        ok = false;
    }

    std::filesystem::remove_all(dir);
    return ok ? 0 : 1;
}
