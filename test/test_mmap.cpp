#include <gtest/gtest.h>

#include <cstdint>
#include <filesystem>
#include <string>
#include <unistd.h>

#include "mmap/mmap.hpp"
#include "utils/filestream_linux.hpp"

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

TEST_F(FsAndMMapTest, PageSizeIsPositive) {
    EXPECT_GT(fs::GetPageSize(), 0u);
}

TEST_F(FsAndMMapTest, DefaultFdGetFdThrows) {
    fs::Fd fd;
    EXPECT_THROW(fd.GetFd(), std::runtime_error);
}

TEST_F(FsAndMMapTest, OpenedFdCanQueryFileSize) {
    fs::Fd fd(file_path_);
    EXPECT_GE(fd.GetFd(), 0);
    EXPECT_EQ(fd.GetFileSize(), 0u);
}

TEST_F(FsAndMMapTest, DoubleOpenThrows) {
    fs::Fd fd(file_path_);
    EXPECT_THROW(fd.Open(file_path_), std::runtime_error);
}

TEST_F(FsAndMMapTest, DefaultMMapDataThrows) {
    fs::MMap mm;
    EXPECT_THROW(mm.Data(), std::runtime_error);
}

TEST_F(FsAndMMapTest, MMapAuxCtorWorks) {
    EXPECT_NO_THROW({
        mmap::MMapAux mm(file_path_);
        EXPECT_TRUE(mm.Empty());
    });
}

}  // namespace
