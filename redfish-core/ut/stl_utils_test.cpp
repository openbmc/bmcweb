#include "utils/stl_utils.hpp"

#include <gmock/gmock.h>

TEST(STLUtilesTest, RemoveDuplicates)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};
    std::vector<std::string> retVec = {"s1", "s4", "s2", "", "s3"};

    strVec.erase(
        redfish::stl_utils::hasDuplicates(strVec.begin(), strVec.end()),
        strVec.end());
    EXPECT_EQ(strVec, retVec);
}
