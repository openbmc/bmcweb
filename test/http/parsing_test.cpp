// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http/parsing.hpp"

#include <format>
#include <string>

#include <gtest/gtest.h>

namespace
{

TEST(HttpParsing, isJsonContentType)
{
    EXPECT_TRUE(isJsonContentType("application/json"));

    // The Redfish specification DSP0266 shows no space between the ; and
    // charset.
    EXPECT_TRUE(isJsonContentType("application/json;charset=utf-8"));
    EXPECT_TRUE(isJsonContentType("application/json;charset=ascii"));

    // Sites like mozilla show the space included [1]
    //  https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Content-Type
    EXPECT_TRUE(isJsonContentType("application/json; charset=utf-8"));

    EXPECT_TRUE(isJsonContentType("APPLICATION/JSON"));
    EXPECT_TRUE(isJsonContentType("APPLICATION/JSON; CHARSET=UTF-8"));
    EXPECT_TRUE(isJsonContentType("APPLICATION/JSON;CHARSET=UTF-8"));

    EXPECT_FALSE(isJsonContentType("application/xml"));
    EXPECT_FALSE(isJsonContentType(""));
    EXPECT_FALSE(isJsonContentType(";"));
    EXPECT_FALSE(isJsonContentType("application/json;"));
    EXPECT_FALSE(isJsonContentType("application/json; "));
    EXPECT_FALSE(isJsonContentType("json"));
}

TEST(HttpParsing, parseRequestAsJsonLimitsArrayDepth)
{
    // 10 levels deep
    std::string veryDeepArray = "[[[[[[[[[[]]]]]]]]]]";

    EXPECT_TRUE(parseStringAsJson(veryDeepArray));

    veryDeepArray = std::format("[{}]", veryDeepArray);
    EXPECT_FALSE(parseStringAsJson(veryDeepArray));
}

TEST(HttpParsing, parseRequestAsJsonLimitsObjectDepths)
{
    // 10 levels deep
    std::string veryDeepObject =
        R"({"1":{"2":{"3":{"4":{"5":{"6":{"7":{"8":{"9":{}}}}}}}}}})";
    EXPECT_FALSE(nlohmann::json::parse(veryDeepObject).is_discarded());
    EXPECT_TRUE(parseStringAsJson(veryDeepObject));

    veryDeepObject = std::format(R"({{"0": {}}})", veryDeepObject);
    EXPECT_FALSE(parseStringAsJson(veryDeepObject));
}

// Makes an array of n elements
std::string makeArray(size_t n)
{
    std::string os;
    os += "[";
    for (size_t i = 0; i < n; ++i)
    {
        if (i != 0)
        {
            os += ",";
        }
        os += std::to_string(i);
    }
    os += "]";
    return os;
}

TEST(HttpParsing, parseStringAsJsonMaxValues)
{
    std::string json500 = makeArray(500);
    EXPECT_TRUE(parseStringAsJson(json500)) << "500 values should parse";

    std::string json501 = makeArray(501);
    EXPECT_FALSE(parseStringAsJson(json501)) << "501 values should be rejected";
}

} // namespace
