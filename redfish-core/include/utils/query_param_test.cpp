#include "query_param.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

TEST(QueryParams, GetExpandType)
{
    redfish::query_param::Query query{};

    EXPECT_FALSE(getExpandType("", query));
    EXPECT_FALSE(getExpandType(".(", query));
    EXPECT_FALSE(getExpandType(".()", query));
    EXPECT_FALSE(getExpandType(".($levels=1", query));

    EXPECT_TRUE(getExpandType("*", query));
    EXPECT_EQ(query.expandType, redfish::query_param::ExpandType::Hyperlinks);
    EXPECT_TRUE(getExpandType(".", query));
    EXPECT_EQ(query.expandType,
              redfish::query_param::ExpandType::NotHyperlinks);
    EXPECT_TRUE(getExpandType("~", query));
    EXPECT_EQ(query.expandType, redfish::query_param::ExpandType::Both);

    // Per redfish specification, level defaults to 1
    EXPECT_TRUE(getExpandType(".", query));
    EXPECT_EQ(query.expandLevel, 1);

    EXPECT_TRUE(getExpandType(".($levels=42)", query));
    EXPECT_EQ(query.expandLevel, 42);

    // Overflow
    EXPECT_FALSE(getExpandType(".($levels=256)", query));

    // Negative
    EXPECT_FALSE(getExpandType(".($levels=-1)", query));

    // No number
    EXPECT_FALSE(getExpandType(".($levels=a)", query));
}