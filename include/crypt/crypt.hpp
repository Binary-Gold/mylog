#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <tuple>

// 不可使用 namespace crypt，会与 <unistd.h> 中 POSIX 函数 crypt 冲突
namespace logcrypt {
    std::tuple<std::string, std::string> GenECDHKey();
    std::string GenECDHSharedSecret(const std::string& client_pri, const std::string& server_pub);

    std::string BinaryKeyToHex(const std::string& binary_key);
    std::string HexKeyToBinary(const std::string& hex_key);

    class Crypt {
    public:
        virtual ~Crypt() = default;

        virtual void Encrypt(const void* input, size_t input_size, std::string& output) = 0;
        virtual std::string Decrypt(const void* data, size_t size) = 0;
    };
}  // namespace logcrypt
