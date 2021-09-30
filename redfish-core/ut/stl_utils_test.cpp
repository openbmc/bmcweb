#include "utils/stl_utils.hpp"

#include <gmock/gmock.h>

TEST(STLUtilesTest, RemoveDuplicates)
{
    std::vector<std::string> strVec = {"s1", "s4", "s1", "s2", "", "s3", "s3"};

    stl_utils::removeDuplicate(strVec);

    EXPECT_EQ(strVec.size(), 5);
    EXPECT_THAT(strVec[0], ::testing::ElementsAre("s1"));
    EXPECT_THAT(strVec[1], ::testing::ElementsAre("s4"));
    EXPECT_THAT(strVec[2], ::testing::ElementsAre("s2"));
    EXPECT_THAT(strVec[3], ::testing::ElementsAre(""));
    EXPECT_THAT(strVec[4], ::testing::ElementsAre("s3"));
}
