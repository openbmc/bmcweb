#include "human_sort.hpp"

#include <set>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

using ::testing::ElementsAreArray;

TEST(AlphaNum, NumberTests)
{
    // testcases for the algorithm
    EXPECT_EQ(alphanumComp("", ""), 0);
    EXPECT_LT(alphanumComp("", "a"), 0);
    EXPECT_GT(alphanumComp("a", ""), 0);
    EXPECT_EQ(alphanumComp("a", "a"), 0);
    EXPECT_LT(alphanumComp("", "9"), 0);
    EXPECT_GT(alphanumComp("9", ""), 0);
    EXPECT_EQ(alphanumComp("1", "1"), 0);
    EXPECT_LT(alphanumComp("1", "2"), 0);
    EXPECT_GT(alphanumComp("3", "2"), 0);
    EXPECT_EQ(alphanumComp("a1", "a1"), 0);
    EXPECT_LT(alphanumComp("a1", "a2"), 0);
    EXPECT_GT(alphanumComp("a2", "a1"), 0);
    EXPECT_LT(alphanumComp("a1a2", "a1a3"), 0);
    EXPECT_GT(alphanumComp("a1a2", "a1a0"), 0);
    EXPECT_GT(alphanumComp("134", "122"), 0);
    EXPECT_EQ(alphanumComp("12a3", "12a3"), 0);
    EXPECT_GT(alphanumComp("12a1", "12a0"), 0);
    EXPECT_LT(alphanumComp("12a1", "12a2"), 0);
    EXPECT_LT(alphanumComp("a", "aa"), 0);
    EXPECT_GT(alphanumComp("aaa", "aa"), 0);
    EXPECT_EQ(alphanumComp("Alpha 2", "Alpha 2"), 0);
    EXPECT_LT(alphanumComp("Alpha 2", "Alpha 2A"), 0);
    EXPECT_GT(alphanumComp("Alpha 2 B", "Alpha 2"), 0);

    std::string str("Alpha 2");
    EXPECT_EQ(alphanumComp(str, "Alpha 2"), 0);
    EXPECT_LT(alphanumComp(str, "Alpha 2A"), 0);
    EXPECT_GT(alphanumComp("Alpha 2 B", str), 0);
}

TEST(AlphaNum, LessTest)
{
    std::set<std::string, AlphanumLess<std::string>> sorted{
        "Alpha 10", "Alpha 2"};
    EXPECT_THAT(sorted, ElementsAreArray({"Alpha 2", "Alpha 10"}));
}
} // namespace
