#include "utils/stl_utils.hpp"

#include <string>

#include <gtest/gtest-message.h>
#include <gtest/gtest-test-part.h>
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
namespace redfish::stl_utils
{
namespace
{

TEST(FirstDuplicate, ReturnsIteratorToFirstDuplicate)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};
    auto iter = firstDuplicate(strVec.begin(), strVec.end());
    EXPECT_EQ(*iter, "s3");
}

TEST(RemoveDuplicates, AllDuplicatesAreRempvedInplace)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};
    removeDuplicate(strVec);

    EXPECT_EQ(strVec.size(), 5);
    EXPECT_EQ(strVec[0], "s1");
    EXPECT_EQ(strVec[1], "s4");
    EXPECT_EQ(strVec[2], "s2");
    EXPECT_EQ(strVec[3], "");
    EXPECT_EQ(strVec[4], "s3");
}
} // namespace
} // namespace redfish::stl_utils
