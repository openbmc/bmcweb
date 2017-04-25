#include "crow/ci_map.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;

TEST(CiMap, MapEmpty) {
  crow::ci_map map;
  EXPECT_TRUE(map.empty());
  EXPECT_EQ(map.size(), 0);

  map.emplace("foo", "bar");

  map.clear();
  EXPECT_TRUE(map.empty());
}

TEST(CiMap, MapBasicInsert) {
  crow::ci_map map;
  map.emplace("foo", "bar");
  auto x = map.find("foo");
  ASSERT_NE(x, map.end());

  EXPECT_EQ(map.find("foo")->first, "foo");
  EXPECT_EQ(map.find("foo")->second, "bar");
  EXPECT_EQ(map.find("FOO")->first, "foo");
  EXPECT_EQ(map.find("FOO")->second, "bar");
}

TEST(CiMap, MapManyInsert) {
  crow::ci_map map;
  map.emplace("foo", "car");
  map.emplace("foo", "boo");
  map.emplace("bar", "cat");
  map.emplace("baz", "bat");

  EXPECT_EQ(map.size(), 3);
  ASSERT_NE(map.find("foo"), map.end());
  EXPECT_EQ(map.find("foo")->first, "foo");
  EXPECT_EQ(map.find("foo")->second, "car");

  ASSERT_NE(map.find("FOO"), map.end());
  EXPECT_EQ(map.find("FOO")->first, "foo");
  EXPECT_EQ(map.find("FOO")->second, "car");
  
  ASSERT_NE(map.find("bar"), map.end());
  EXPECT_EQ(map.find("bar")->first, "bar");
  EXPECT_EQ(map.find("bar")->second, "cat");

  ASSERT_NE(map.find("BAR"), map.end());
  EXPECT_EQ(map.find("BAR")->first, "bar");
  EXPECT_EQ(map.find("BAR")->second, "cat");

  ASSERT_NE(map.find("baz"), map.end());
  EXPECT_EQ(map.find("baz")->first, "baz");
  EXPECT_EQ(map.find("baz")->second, "bat");

  ASSERT_NE(map.find("BAZ"), map.end());
  EXPECT_EQ(map.find("BAZ")->first, "baz");
  EXPECT_EQ(map.find("BAZ")->second, "bat");

  EXPECT_EQ(map.count("foo"), 1);
  EXPECT_EQ(map.count("bar"), 1);
  EXPECT_EQ(map.count("baz"), 1);
  EXPECT_EQ(map.count("FOO"), 1);
  EXPECT_EQ(map.count("BAR"), 1);
  EXPECT_EQ(map.count("BAZ"), 1);
}

TEST(CiMap, MapMultiInsert) {
  crow::ci_map map;
  map.emplace("foo", "bar1");
  map.emplace("foo", "bar2");
  EXPECT_EQ(map.count("foo"), 1);
  EXPECT_EQ(map.count("FOO"), 1);
  EXPECT_EQ(map.count("fOo"), 1);
  EXPECT_EQ(map.count("FOo"), 1);
}