#include "base64.hpp"
#include "big_list_of_naughty_strings.hpp"
#include "gtest/gtest.h"

// Tests that Base64 basic strings work
TEST(Base64, EncodeBasicString) {
  std::string output;
  EXPECT_TRUE(base64::base64_encode("Foo", output));
}

// Tests the test vectors available in the base64 spec
TEST(Base64, EncodeRFC4648) {
  std::string output;
  EXPECT_TRUE(base64::base64_encode("", output));
  EXPECT_EQ(output, "");
  EXPECT_TRUE(base64::base64_encode("f", output));
  EXPECT_EQ(output, "Zg==");
  EXPECT_TRUE(base64::base64_encode("fo", output));
  EXPECT_EQ(output, "Zm8=");
  EXPECT_TRUE(base64::base64_encode("foo", output));
  EXPECT_EQ(output, "Zm9v");
  EXPECT_TRUE(base64::base64_encode("foob", output));
  EXPECT_EQ(output, "Zm9vYg==");
  EXPECT_TRUE(base64::base64_encode("fooba", output));
  EXPECT_EQ(output, "Zm9vYmE=");
  EXPECT_TRUE(base64::base64_encode("foobar", output));
  EXPECT_EQ(output, "Zm9vYmFy");
}

// Tests the test vectors available in the base64 spec
TEST(Base64, DecodeRFC4648) {
  std::string output;
  EXPECT_TRUE(base64::base64_decode("", output));
  EXPECT_EQ(output, "");
  EXPECT_TRUE(base64::base64_decode("Zg==", output));
  EXPECT_EQ(output, "f");
  EXPECT_TRUE(base64::base64_decode("Zm8=", output));
  EXPECT_EQ(output, "fo");
  EXPECT_TRUE(base64::base64_decode("Zm9v", output));
  EXPECT_EQ(output, "foo");
  EXPECT_TRUE(base64::base64_decode("Zm9vYg==", output));
  EXPECT_EQ(output, "foob");
  EXPECT_TRUE(base64::base64_decode("Zm9vYmE=", output));
  EXPECT_EQ(output, "fooba");
  EXPECT_TRUE(base64::base64_decode("Zm9vYmFy", output));
  EXPECT_EQ(output, "foobar");
}

// Tests using pathalogical cases for all escapings
TEST(Base64, NaugtyStringsEncodeDecode) {
  std::string base64_string;
  std::string decoded_string;
  for (auto& str : naughty_strings) {
    EXPECT_TRUE(base64::base64_encode(str, base64_string));
    EXPECT_TRUE(base64::base64_decode(base64_string, decoded_string));
    EXPECT_EQ(str, decoded_string);
  }
}

// Tests using pathalogical cases for all escapings
TEST(Base64, NaugtyStringsPathological) {
  std::string base64_string;
  std::string decoded_string;
  for (auto& str : naughty_strings) {
    base64::base64_decode(str, base64_string);
  }
}