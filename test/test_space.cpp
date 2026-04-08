#include <gtest/gtest.h>
#include <utils/space.hpp>

namespace {

using namespace space;

TEST(SpaceTest, DefaultConstructBytesIsZero) {
    bytes b;
    EXPECT_EQ(b.count(), 0u);
}

TEST(SpaceTest, ConstructFromRep) {
    bytes b(42);
    EXPECT_EQ(b.count(), 42u);
}

TEST(SpaceTest, SpaceCastKilobytesToBytes) {
    kilobytes kb(2);
    bytes out = space_cast<bytes>(kb);
    EXPECT_EQ(out.count(), 2048u);
}

TEST(SpaceTest, SpaceCastMegabytesToBytes) {
    megabytes mb(1);
    bytes out = space_cast<bytes>(mb);
    EXPECT_EQ(out.count(), static_cast<uint64_t>(1024) * 1024u);
}

TEST(SpaceTest, UnaryPlusMinus) {
    kilobytes kb(3);
    EXPECT_EQ((+kb).count(), 3u);
    EXPECT_EQ((-kb).count(), static_cast<uint64_t>(-3));
}

TEST(SpaceTest, BinaryPlusMinus) {
    bytes a(10);
    bytes b(4);
    EXPECT_EQ((a + b).count(), 14u);
    EXPECT_EQ((a - b).count(), 6u);
}

TEST(SpaceTest, CompoundAssignment) {
    bytes x(5);
    x += bytes(3);
    EXPECT_EQ(x.count(), 8u);
    x -= bytes(2);
    EXPECT_EQ(x.count(), 6u);
}

TEST(SpaceTest, IncrementDecrement) {
    bytes x(0);
    ++x;
    EXPECT_EQ(x.count(), 1u);
    x++;
    EXPECT_EQ(x.count(), 2u);
    --x;
    EXPECT_EQ(x.count(), 1u);
    x--;
    EXPECT_EQ(x.count(), 0u);
}

}  // namespace
