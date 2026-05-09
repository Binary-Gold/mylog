#include <gtest/gtest.h>

#include <string>

#include "logger/crypt/aes_crypt.hpp"
#include "logger/crypt/crypt.hpp"

namespace {

using namespace logger;

// ==================== ECDH 密钥协商 ====================

TEST(ECDHTest, GenerateKeyPair) {
    auto [pri, pub] = crypt::GenECDHKey();
    EXPECT_FALSE(pri.empty());
    EXPECT_FALSE(pub.empty());
    EXPECT_NE(pri, pub);  // 公私钥不应相同
}

TEST(ECDHTest, GenerateKeyPairDeterministic) {
    // 每次生成应是不同的密钥对
    auto [pri1, pub1] = crypt::GenECDHKey();
    auto [pri2, pub2] = crypt::GenECDHKey();
    EXPECT_NE(pri1, pri2);
    EXPECT_NE(pub1, pub2);
}

TEST(ECDHTest, SharedSecretSymmetric) {
    // 双方各自生成密钥对
    auto [client_pri, client_pub] = crypt::GenECDHKey();
    auto [server_pri, server_pub] = crypt::GenECDHKey();

    // 各自计算共享密钥
    auto client_shared = crypt::GenECDHSharedSecret(client_pri, server_pub);
    auto server_shared = crypt::GenECDHSharedSecret(server_pri, client_pub);

    EXPECT_FALSE(client_shared.empty());
    EXPECT_EQ(client_shared, server_shared);  // ECDH 核心属性：双方计算应一致
}

TEST(ECDHTest, SharedSecretDifferentForDifferentPeers) {
    auto [alice_pri, alice_pub] = crypt::GenECDHKey();
    auto [bob_pri, bob_pub] = crypt::GenECDHKey();
    auto [eve_pri, eve_pub] = crypt::GenECDHKey();

    auto alice_bob = crypt::GenECDHSharedSecret(alice_pri, bob_pub);
    auto alice_eve = crypt::GenECDHSharedSecret(alice_pri, eve_pub);

    EXPECT_NE(alice_bob, alice_eve);
}

// ==================== Hex 编解码 ====================

TEST(HexTest, BinaryKeyToHexAndBack) {
    std::string original = "test_key_1234567";  // 16 bytes
    auto hex = crypt::BinaryKeyToHex(original);
    auto binary = crypt::HexKeyToBinary(hex);
    EXPECT_EQ(original, binary);
}

TEST(HexTest, EmptyBinary) {
    auto hex = crypt::BinaryKeyToHex("");
    EXPECT_TRUE(hex.empty());
}

// ==================== AES 加密 ====================

class AESTest : public ::testing::Test {
public:
    crypt::AESCrypt aes{crypt::AESCrypt::GenerateKey()};
};

TEST_F(AESTest, GenerateKeyProducesValidHex) {
    auto key = crypt::AESCrypt::GenerateKey();
    EXPECT_FALSE(key.empty());
    EXPECT_EQ(key.size(), 32);  // 16 bytes → 32 hex chars
    // 应能正确 round-trip hex↔binary
    auto binary = crypt::HexKeyToBinary(key);
    EXPECT_EQ(binary.size(), 16);
}

TEST_F(AESTest, EncryptDecryptRoundTrip) {
    std::string plain = "Hello, ECDH + AES!";
    std::string cipher;
    aes.Encrypt(plain.data(), plain.size(), cipher);
    EXPECT_FALSE(cipher.empty());

    auto decrypted = aes.Decrypt(cipher.data(), cipher.size());
    EXPECT_EQ(decrypted, plain);
}

TEST_F(AESTest, EmptyInput) {
    std::string cipher;
    aes.Encrypt("", 0, cipher);
    auto decrypted = aes.Decrypt(cipher.data(), cipher.size());
    EXPECT_EQ(decrypted, "");
}

TEST_F(AESTest, BinaryData) {
    std::string plain;
    for (int i = 0; i < 256; ++i)
        plain.push_back(static_cast<char>(i));

    std::string cipher;
    aes.Encrypt(plain.data(), plain.size(), cipher);
    auto decrypted = aes.Decrypt(cipher.data(), cipher.size());
    EXPECT_EQ(decrypted, plain);
}

TEST_F(AESTest, DifferentKeyProducesDifferentCipher) {
    auto aes2 = crypt::AESCrypt(crypt::AESCrypt::GenerateKey());
    std::string plain = "same input";
    std::string c1, c2;
    aes.Encrypt(plain.data(), plain.size(), c1);
    aes2.Encrypt(plain.data(), plain.size(), c2);
    EXPECT_NE(c1, c2);
}

TEST_F(AESTest, WrongKeyDecryptThrows) {
    auto other = crypt::AESCrypt(crypt::AESCrypt::GenerateKey());
    std::string plain = "secret";
    std::string cipher;
    aes.Encrypt(plain.data(), plain.size(), cipher);

    // 错误密钥解密时 Crypto++ 的 PKCS#7 填充校验会抛异常
    EXPECT_THROW(other.Decrypt(cipher.data(), cipher.size()), std::exception);
}

// ==================== ECDH + AES 组合流程 ====================

TEST(CombinedTest, FullECDHAndAESRoundTrip) {
    // 1. 本端生成密钥对
    auto [my_pri, my_pub] = crypt::GenECDHKey();
    // 2. 模拟对方生成密钥对
    auto [peer_pri, peer_pub] = crypt::GenECDHKey();
    // 3. 本端计算共享密钥
    auto my_shared = crypt::GenECDHSharedSecret(my_pri, peer_pub);
    // 4. 对方计算共享密钥（应一致）
    auto peer_shared = crypt::GenECDHSharedSecret(peer_pri, my_pub);
    EXPECT_EQ(my_shared, peer_shared);

    // 5. 用共享密钥创建 AES（取前 16 字节作为 AES-128 key）
    auto aes_key = crypt::BinaryKeyToHex(my_shared.substr(0, 16));
    crypt::AESCrypt my_aes(aes_key);
    crypt::AESCrypt peer_aes(aes_key);

    // 6. 加密 → 解密
    std::string plain = "ECDH + AES combined test payload";
    std::string cipher;
    my_aes.Encrypt(plain.data(), plain.size(), cipher);
    auto decrypted = peer_aes.Decrypt(cipher.data(), cipher.size());
    EXPECT_EQ(decrypted, plain);
}

TEST(CombinedTest, EncryptedDataCannotBeReadWithoutSharedSecret) {
    auto [alice_pri, alice_pub] = crypt::GenECDHKey();
    auto [bob_pri, bob_pub] = crypt::GenECDHKey();
    auto [eve_pri, eve_pub] = crypt::GenECDHKey();

    // Alice 和 Bob 的共享密钥
    auto alice_shared = crypt::GenECDHSharedSecret(alice_pri, bob_pub);
    // Eve 尝试用自己的密钥 + Bob 的公钥
    auto eve_shared = crypt::GenECDHSharedSecret(eve_pri, bob_pub);

    EXPECT_NE(alice_shared, eve_shared);

    auto alice_key = crypt::BinaryKeyToHex(alice_shared.substr(0, 16));
    auto eve_key = crypt::BinaryKeyToHex(eve_shared.substr(0, 16));

    crypt::AESCrypt alice_aes(alice_key);
    crypt::AESCrypt eve_aes(eve_key);

    std::string plain = "message from Alice to Bob";
    std::string cipher;
    alice_aes.Encrypt(plain.data(), plain.size(), cipher);

    // Eve 无法解密（错误密钥导致 PKCS#7 填充校验失败，抛异常）
    EXPECT_THROW(eve_aes.Decrypt(cipher.data(), cipher.size()), std::exception);
}

}  // namespace
