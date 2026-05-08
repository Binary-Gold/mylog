#include <gtest/gtest.h>

#include <cstring>

#include "logger/mmap/mmap.hpp"
#include "logger/utils/filestream_linux.hpp"

namespace {

class FsAndMMapTest : public ::testing::Test {
protected:
    void SetUp() override {
        const auto unique = "mylog_mmap_test_" +
            std::to_string(static_cast<unsigned long long>(::getpid())) + "_" +
            std::to_string(static_cast<unsigned long long>(++counter_));
        file_path_ = std::filesystem::temp_directory_path() / (unique + ".bin");
        std::filesystem::remove(file_path_);
    }

    void TearDown() override {
        std::filesystem::remove(file_path_);
    }

    std::filesystem::path file_path_;
    static inline std::uint64_t counter_ = 0;
};

// ==================== Fd 测试 ====================

TEST_F(FsAndMMapTest, FdConstructOpensFile) {
    logger::fs::Fd fd(file_path_);
    EXPECT_GE(fd.GetFd(), 0);                    // fd 有效
    EXPECT_TRUE(std::filesystem::exists(file_path_)); // 文件被创建
}

TEST_F(FsAndMMapTest, FdEmptyFileSizeIsZero) {
    logger::fs::Fd fd(file_path_);
    EXPECT_EQ(fd.GetFileSize(), 0u);             // 新文件大小为 0
}

// ==================== MMap 测试 ====================

TEST_F(FsAndMMapTest, MMapConstructMapsMemory) {
    logger::fs::Fd fd(file_path_);
    logger::fs::MMap mm(fd, 4096);
    EXPECT_NO_THROW(mm.Data());                  // Data() 不抛异常
}
// ==================== GetPageSize 测试 ====================

TEST_F(FsAndMMapTest, GetPageSize) {
    EXPECT_GE(logger::fs::GetPageSize(), 1);                  
}

// ==================== MMapAux 测试 ====================

TEST_F(FsAndMMapTest, MMapAuxNewlyCreatedIsEmpty) {
    logger::mmap::MMapAux mm(file_path_);
    EXPECT_TRUE(mm.Empty());
    EXPECT_EQ(mm.Size(), 0u);
}

TEST_F(FsAndMMapTest, MMapAuxPushIncreasesSize) {
    logger::mmap::MMapAux mm(file_path_);
    std::string data = "hello";
    mm.Push(data.c_str(), data.size());
    EXPECT_EQ(mm.Size(), 5u);
}

TEST_F(FsAndMMapTest, MMapAuxPushDataReadBack) {
    logger::mmap::MMapAux mm(file_path_);
    std::string data = "hello mmap";
    mm.Push(data.c_str(), data.size());
    EXPECT_EQ(std::memcmp(mm.Data(), data.c_str(), data.size()), 0);
}

TEST_F(FsAndMMapTest, MMapAuxClearMakesEmpty) {
    logger::mmap::MMapAux mm(file_path_);
    mm.Push("x", 1);
    mm.Clear();
    EXPECT_TRUE(mm.Empty());
    EXPECT_EQ(mm.Size(), 0u);
}

TEST_F(FsAndMMapTest, MMapAuxMultiplePushAccumulates) {
    logger::mmap::MMapAux mm(file_path_);
    mm.Push("abc", 3);
    mm.Push("def", 3);
    EXPECT_EQ(mm.Size(), 6u);
    EXPECT_EQ(std::memcmp(mm.Data(), "abcdef", 6), 0);
}

}  // namespace
