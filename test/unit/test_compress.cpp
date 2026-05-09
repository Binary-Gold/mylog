#include <gtest/gtest.h>

#include <string>
#include <vector>

#include "logger/compress/zlib_compress.hpp"
#include "logger/compress/zstd_compress.hpp"

namespace {

using namespace logger;

// ---- 模板类: 对 Zlib/Zstd 运行同一套测试 ----
template <typename T>
class CompressTest : public ::testing::Test {
public:
    T compressor;
    std::string compress(const std::string& input) {
        std::string out;
        out.resize(compressor.CompressedBound(input.size()));
        size_t real = compressor.Compress(input.data(), input.size(), out.data(), out.size());
        out.resize(real);
        return out;
    }
    std::string decompress(const void* data, size_t size) {
        return compressor.Decompress(data, size);
    }
};

using CompressTypes = ::testing::Types<compress::ZlibCompress, compress::ZstdCompress>;
// using CompressTypes = ::testing::Types<compress::ZstdCompress>;

TYPED_TEST_SUITE(CompressTest, CompressTypes);

// ==================== 基础 Compress ====================

TYPED_TEST(CompressTest, NullInputReturnsZero) {
    char buf[256];
    EXPECT_EQ(this->compressor.Compress(nullptr, 10, buf, sizeof(buf)), 0);
}

TYPED_TEST(CompressTest, NullOutputReturnsZero) {
    const char input[] = "hello";
    EXPECT_EQ(this->compressor.Compress(input, 5, nullptr, 256), 0);
}

TYPED_TEST(CompressTest, EmptyInput) {
    std::string input;
    std::string out;
    out.resize(this->compressor.CompressedBound(0));
    size_t real = this->compressor.Compress(input.data(), 0, out.data(), out.size());
    out.resize(real);
    // 空输入压缩后解压应返回空字符串
    auto decompressed = this->compressor.Decompress(out.data(), out.size());
    EXPECT_EQ(decompressed, "");
}

TYPED_TEST(CompressTest, CompressedBoundIsSufficient) {
    std::string input(4096, 'x');
    std::string out;
    out.resize(this->compressor.CompressedBound(input.size()));
    size_t real = this->compressor.Compress(input.data(), input.size(), out.data(), out.size());
    // 不能返回 0（表示错误），且不能超过预分配大小
    EXPECT_GT(real, 0);
    EXPECT_LE(real, out.size());
}

// ==================== 往返 (Round-Trip) ====================

TYPED_TEST(CompressTest, BasicRoundTrip) {
    std::string original = "Hello, World! This is a compression test.";
    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed, original);
}

TYPED_TEST(CompressTest, RepeatedStringCompressesSmaller) {
    // 高重复度数据，压缩后应明显缩小
    std::string original(4000, 'A');  // 4000 个 'A'

    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());

    EXPECT_EQ(decompressed, original);
    EXPECT_LT(compressed.size(), original.size()) << "重复数据应该被压缩得更小";
}

TYPED_TEST(CompressTest, RandomDataRoundTrip) {
    // 即使不可压缩数据，往返后也应正确
    std::string original;
    original.reserve(1024);
    for (int i = 0; i < 1024; ++i)
        original.push_back(static_cast<char>((i * 37 + 13) % 256));

    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());

    EXPECT_EQ(decompressed, original);
}

TYPED_TEST(CompressTest, BinaryDataRoundTrip) {
    std::string original;
    original.reserve(256);
    for (int i = 0; i < 256; ++i)
        original.push_back(static_cast<char>(i));  // 0x00–0xFF

    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());

    EXPECT_EQ(decompressed, original);
}

// ==================== 确定性 ====================

TYPED_TEST(CompressTest, DeterministicCompression) {
    std::string input = "determinism test data";
    std::string compressed1 = this->compress(input);
    std::string compressed2 = this->compress(input);
    EXPECT_EQ(compressed1, compressed2);
}

// ==================== 多次压缩 (流复用) ====================

TYPED_TEST(CompressTest, MultipleCompressCalls) {
    std::vector<std::string> inputs = {
        "first chunk of data",
        "second chunk, different content",
        "third: aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa",  // 高重复
    };

    for (const auto& original : inputs) {
        auto compressed = this->compress(original);
        auto decompressed = this->decompress(compressed.data(), compressed.size());
        EXPECT_EQ(decompressed, original);
    }
}

// ==================== 大块数据 ====================

TYPED_TEST(CompressTest, LargeDataRoundTrip) {
    std::string original(1 << 20, 'B');  // 1MB 重复数据
    // 隔一段掺一点变化，避免太"假"
    for (size_t i = 0; i < original.size(); i += 1000)
        original[i] = static_cast<char>('a' + (i % 26));

    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());

    EXPECT_EQ(decompressed, original);
    EXPECT_LT(compressed.size(), original.size());
}

// ==================== 单字符 ====================

TYPED_TEST(CompressTest, SingleCharacter) {
    std::string original = "X";
    auto compressed = this->compress(original);
    auto decompressed = this->decompress(compressed.data(), compressed.size());
    EXPECT_EQ(decompressed, original);
}

// ==================== 解压异常输入 ====================

TYPED_TEST(CompressTest, DecompressNullData) {
    auto result = this->compressor.Decompress(nullptr, 0);
    EXPECT_TRUE(result.empty());
}

TYPED_TEST(CompressTest, DecompressZeroSize) {
    const char dummy = 'x';
    auto result = this->compressor.Decompress(&dummy, 0);
    EXPECT_TRUE(result.empty());
}

TYPED_TEST(CompressTest, DecompressGarbageData) {
    std::string garbage(128, '\x00');
    // 垃圾数据解压: zlib 返回空输入, zstd 可能抛异常, 不崩溃即可
    try {
        auto result = this->compressor.Decompress(garbage.data(), garbage.size());
        (void)result;
    } catch (const std::exception&) {
        // zstd throws on invalid data — expected
    }
}

// ==================== 边界条件 ====================

TYPED_TEST(CompressTest, OutputBufferExactBound) {
    std::string original(1024, 'c');
    std::string out;
    // 恰好等于 CompressedBound 的空间
    out.resize(this->compressor.CompressedBound(original.size()));
    size_t real = this->compressor.Compress(original.data(), original.size(),
                                            out.data(), out.size());
    EXPECT_GT(real, 0);
    EXPECT_LE(real, out.size());
    out.resize(real);
    auto decompressed = this->compressor.Decompress(out.data(), out.size());
    EXPECT_EQ(decompressed, original);
}

// ==================== Zstd-specific: 无效数据抛异常 ====================

TEST(ZstdCompressTest, DecompressInvalidDataThrows) {
    compress::ZstdCompress zstd;
    std::string garbage(128, '\xAA');
    EXPECT_THROW(zstd.Decompress(garbage.data(), garbage.size()), std::runtime_error);
}

// ==================== Zlib-specific: CompressedBound 实际大小 ====================

TEST(ZlibCompressTest, CompressedBoundSufficient) {
    compress::ZlibCompress zlib;
    // CompressedBound 必须 >= compressBound()，保证缓冲区足够
    EXPECT_GE(zlib.CompressedBound(0), 0);
    EXPECT_GE(zlib.CompressedBound(100), 100);
    EXPECT_GE(zlib.CompressedBound(65536), 65536);
    EXPECT_GE(zlib.CompressedBound(1 << 20), 1 << 20);
}

}  // namespace
