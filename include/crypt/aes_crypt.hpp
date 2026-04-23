#pragma once

#include <memory>
#include <string>

#include "crypt/crypt.hpp"

namespace logger {
namespace crypt {
    class AESCrypt final : public Crypt {
    public:
        explicit AESCrypt(std::string key);
        ~AESCrypt() override;

        static std::string GenerateKey();
        static std::string GenerateIV();

        void Encrypt(const void* input, size_t input_size, std::string& output) override;
        std::string Decrypt(const void* data, size_t size) override;

    private:
        struct Imp;
        std::unique_ptr<Imp> imp_;
    };
}  // namespace crypt
}  // namespace logger
