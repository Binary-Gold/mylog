#include <gtest/gtest.h>
#include "logger/utils/space.hpp"

namespace {

using namespace logger;

// ==================== 构造 ====================

TEST(SpaceTest, DefaultConstructIsZero) {
    bytes b;
    EXPECT_EQ(b.count(), 0u);
}

TEST(SpaceTest, ConstructFromRep) {
    bytes b(42);
    EXPECT_EQ(b.count(), 42u);
}

TEST(SpaceTest, ConstructFromDifferentUnit) {
    kilobytes kb(2);                       // 2 KB = 2048 字节
    bytes b = kb;                          // 隐式转换走 space_cast
    EXPECT_EQ(b.count(), 1024u * 2);
}

// ==================== count ====================

TEST(SpaceTest, CountReturnsStoredValue) {
    bytes b(100);
    EXPECT_EQ(b.count(), 100u);
    EXPECT_EQ(kilobytes(3).count(), 3u);
}

// ==================== 自增、自减 ====================

TEST(SpaceTest, PrefixIncrement) {
    bytes b(0);
    EXPECT_EQ((++b).count(), 1u);
    EXPECT_EQ(b.count(), 1u);
}

TEST(SpaceTest, PostfixIncrement) {
    bytes b(0);
    EXPECT_EQ((b++).count(), 0u);
    EXPECT_EQ(b.count(), 1u);
}

TEST(SpaceTest, PrefixDecrement) {
    bytes b(5);
    EXPECT_EQ((--b).count(), 4u);
    EXPECT_EQ(b.count(), 4u);
}

TEST(SpaceTest, PostfixDecrement) {
    bytes b(5);
    EXPECT_EQ((b--).count(), 5u);
    EXPECT_EQ(b.count(), 4u);
}

// ==================== 复合赋值 ====================

TEST(SpaceTest, AddAssign) {
    bytes b(5);
    b += bytes(3);
    EXPECT_EQ(b.count(), 8u);
}

TEST(SpaceTest, SubAssign) {
    bytes b(10);
    b -= bytes(4);
    EXPECT_EQ(b.count(), 6u);
}

TEST(SpaceTest, MulAssign) {
    bytes b(5);
    b *= bytes(3);
    EXPECT_EQ(b.count(), 15u);
}

TEST(SpaceTest, DivAssign) {
    bytes b(20);
    b /= bytes(4);
    EXPECT_EQ(b.count(), 5u);
}

TEST(SpaceTest, ModAssign) {
    bytes b(20);
    b %= bytes(7);
    EXPECT_EQ(b.count(), 6u);
}

// ==================== 二元运算符（非成员） ====================

TEST(SpaceTest, BinaryAdd) {
    bytes a(10);
    bytes b(4);
    EXPECT_EQ((a + b).count(), 14u);
}

TEST(SpaceTest, BinarySub) {
    bytes a(10);
    bytes b(4);
    EXPECT_EQ((a - b).count(), 6u);
}

// ==================== space_cast ====================

TEST(SpaceTest, KilobytesToBytes) {
    kilobytes kb(3);
    bytes b = space_cast<bytes>(kb);
    EXPECT_EQ(b.count(), 1024u * 3);
}

TEST(SpaceTest, MegabytesToBytes) {
    megabytes mb(1);
    bytes b = space_cast<bytes>(mb);
    EXPECT_EQ(b.count(), 1024u * 1024);
}

TEST(SpaceTest, GigabytesToBytes) {
    gigabytes gb(1);
    bytes b = space_cast<bytes>(gb);
    EXPECT_EQ(b.count(), 1024u * 1024 * 1024);
}

TEST(SpaceTest, BytesToKilobytesTruncates) {
    bytes b(2047);                     // 离 2 KB 差 1 字节
    kilobytes kb = space_cast<kilobytes>(b);
    EXPECT_EQ(kb.count(), 1u);         // 整数除法截断: 2047 / 1024 = 1
}

TEST(SpaceTest, MegabytesToKilobytes) {
    megabytes mb(2);
    kilobytes kb = space_cast<kilobytes>(mb);
    EXPECT_EQ(kb.count(), 2048u);      // 2 × 1048576 / 1024 = 2048
}

// ==================== 预定义类型别名 ====================

TEST(SpaceTest, TypeAliasBytes) {
    bytes b(1);
    EXPECT_EQ(b.count(), 1u);
}

TEST(SpaceTest, TypeAliasKilobytes) {
    kilobytes kb(1);
    bytes b = space_cast<bytes>(kb);
    EXPECT_EQ(b.count(), 1024u);
}

TEST(SpaceTest, TypeAliasMegabytes) {
    megabytes mb(1);
    bytes b = space_cast<bytes>(mb);
    EXPECT_EQ(b.count(), 1048576u);
}

TEST(SpaceTest, TypeAliasTerabytes) {
    terabytes tb(1);
    bytes b = space_cast<bytes>(tb);
    EXPECT_EQ(b.count(), 1099511627776u);
}

// ==================== 行为一致性 ====================

TEST(SpaceTest, CopyConstruction) {
    bytes a(10);
    bytes b = a;
    EXPECT_EQ(b.count(), 10u);
}

TEST(SpaceTest, CopyAssignment) {
    bytes a(10);
    bytes b;
    b = a;
    EXPECT_EQ(b.count(), 10u);
}

TEST(SpaceTest, EmptyIsZero) {
    bytes b;
    EXPECT_EQ(b.count(), 0u);
}

TEST(SpaceTest, LargeValue) {
    megabytes mb(4096);                 // 4 GB
    bytes b = space_cast<bytes>(mb);
    EXPECT_EQ(b.count(), 4294967296u);
}

}  // namespace
