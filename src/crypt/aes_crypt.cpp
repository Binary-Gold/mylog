#include "crypt/aes_crypt.hpp"

#include <utility>

#include "../../third_party/cryptopp/aes.h"
#include "../../third_party/cryptopp/base64.h"
#include "../../third_party/cryptopp/cryptlib.h"
#include "../../third_party/cryptopp/eccrypto.h"
#include "../../third_party/cryptopp/filters.h"
#include "../../third_party/cryptopp/hex.h"
#include "../../third_party/cryptopp/modes.h"
#include "../../third_party/cryptopp/oids.h"
#include "../../third_party/cryptopp/osrng.h"

namespace logger {
namespace crypt {
    struct AESCrypt::Imp {
        std::string key_;
        std::string iv_;
    };

    namespace detail {
        using CryptoPP::byte;

        std::string GenerateKey() {
            CryptoPP::AutoSeededRandomPool rnd;
            byte key[CryptoPP::AES::DEFAULT_KEYLENGTH];
            rnd.GenerateBlock(key, sizeof(key));
            return BinaryKeyToHex(std::string(reinterpret_cast<const char*>(key), sizeof(key)));
        }

        std::string GenerateIV() {
            CryptoPP::AutoSeededRandomPool rnd;
            byte iv[CryptoPP::AES::BLOCKSIZE];
            rnd.GenerateBlock(iv, sizeof(iv));
            return BinaryKeyToHex(std::string(reinterpret_cast<const char*>(iv), sizeof(iv)));
        }

        void Encrypt(const void* input, size_t input_size, std::string& output, const std::string& key,
                     const std::string& iv) {
            CryptoPP::AES::Encryption aes_encryption(reinterpret_cast<const byte*>(key.data()), key.size());
            CryptoPP::CBC_Mode_ExternalCipher::Encryption cbc_encryption(
                aes_encryption, reinterpret_cast<const byte*>(iv.data()));
            CryptoPP::StreamTransformationFilter stf_encryptor(cbc_encryption, new CryptoPP::StringSink(output));
            stf_encryptor.Put(reinterpret_cast<const byte*>(input), input_size);
            stf_encryptor.MessageEnd();
        }

        std::string Decrypt(const void* data, size_t size, const std::string& key, const std::string& iv) {
            std::string decrypted_text;
            CryptoPP::AES::Decryption aes_decryption(reinterpret_cast<const byte*>(key.data()), key.size());
            CryptoPP::CBC_Mode_ExternalCipher::Decryption cbc_decryption(
                aes_decryption, reinterpret_cast<const byte*>(iv.data()));
            CryptoPP::StreamTransformationFilter stf_decryptor(cbc_decryption,
                                                               new CryptoPP::StringSink(decrypted_text));
            stf_decryptor.Put(reinterpret_cast<const byte*>(data), size);
            stf_decryptor.MessageEnd();
            return decrypted_text;
        }
    }  // namespace detail

    AESCrypt::AESCrypt(std::string key) : imp_(std::make_unique<Imp>()) {
        imp_->key_ = std::move(key);
        imp_->iv_ = "dad0c0012340080a";
    }

    AESCrypt::~AESCrypt() = default;

    void AESCrypt::Encrypt(const void* input, size_t input_size, std::string& output) {
        detail::Encrypt(input, input_size, output, imp_->key_, imp_->iv_);
    }

    std::string AESCrypt::Decrypt(const void* data, size_t size) {
        return detail::Decrypt(data, size, imp_->key_, imp_->iv_);
    }

    std::string AESCrypt::GenerateKey() {
        return detail::GenerateKey();
    }

    std::string AESCrypt::GenerateIV() {
        return detail::GenerateIV();
    }
}  // namespace crypt
}  // namespace logger
