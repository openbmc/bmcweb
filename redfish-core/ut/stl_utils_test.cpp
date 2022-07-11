#include "utils/stl_utils.hpp"

#include <string>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"

namespace redfish::stl_utils
{
namespace
{
using ::testing::ElementsAre;

TEST(FirstDuplicate, ReturnsIteratorToFirstDuplicate)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};
    auto iter = firstDuplicate(strVec.begin(), strVec.end());
    ASSERT_NE(iter, strVec.end());
    EXPECT_EQ(*iter, "s3");
}

TEST(RemoveDuplicates, AllDuplicatesAreRempvedInplace)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};
    removeDuplicate(strVec);

    EXPECT_THAT(strVec, ElementsAre("s1", "s4", "s2", "", "s3"));
}
} // namespace
} // namespace redfish::stl_utils
