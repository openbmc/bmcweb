#include <string>
#include "gtest/gtest.h"

TEST(MemorySanitizer, TestIsWorking) {
  std::string foo("foo");
  EXPECT_STREQ("foo", foo.c_str());
}
