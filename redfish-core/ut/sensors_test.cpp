#include "sensors.hpp"
#include "utils/query_param.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace redfish::sensors
{
namespace
{

TEST(IsEfficientExpandAvailable, Positive)
{
    query_param::Query query;
    query.expandLevel = 1;
    query.expandType = query_param::ExpandType::Both;
    EXPECT_TRUE(isEfficientExpandAvailable(query));

    query.expandType = query_param::ExpandType::Links;
    EXPECT_TRUE(isEfficientExpandAvailable(query));

    query.expandType = query_param::ExpandType::NotLinks;
    EXPECT_TRUE(isEfficientExpandAvailable(query));
}

TEST(IsEfficientExpandAvailable, Negative)
{
    query_param::Query query;

    // Expand is level 2
    query.isOnly = false;
    query.expandLevel = 2;
    query.expandType = query_param::ExpandType::Links;
    EXPECT_FALSE(isEfficientExpandAvailable(query));

    // Not an expand query
    query.expandLevel = 1;
    query.expandType = query_param::ExpandType::None;
    EXPECT_FALSE(isEfficientExpandAvailable(query));
}
} // namespace
} // namespace redfish::sensors
