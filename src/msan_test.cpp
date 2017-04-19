#include "gtest/gtest.h"
#include <string>

TEST(MemorySanitizer, TestIsWorking) {
  std::string foo("foo");
  EXPECT_STREQ("foo", foo.c_str());
}
