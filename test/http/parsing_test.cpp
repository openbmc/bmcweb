// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http/parsing.hpp"

#include <format>
#include <string>

#include <gtest/gtest.h>

namespace
{

// Makes an array of n elements
std::string makeWideArray(int n)
{
    n -= 1; // The array itself counts as one element
    std::string os;
    os += "[";
    for (int i = 0; i < n; ++i)
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

std::string makeDeepArray(int n)
{
    std::string os;
    for (int i = 0; i < n; ++i)
    {
        os += "[";
    }
    for (int i = 0; i < n; ++i)
    {
        os += "]";
    }
    return os;
}
// Makes an object of n elements deep
std::string makeDeepObject(int n)
{
    std::string os;
    for (int i = 0; i < n; ++i)
    {
        os += std::format(R"({{"{}": )", i);
    }
    os += "{";
    for (int i = 0; i < n + 1; ++i)
    {
        os += "}";
    }
    return os;
}

std::string makeWideObject(int n)
{
    std::string os;
    os += "{";
    for (int i = 0; i < n; ++i)
    {
        os += std::format(R"("{}": {})", i, i);
        if (i != n - 1)
        {
            os += ",";
        }
    }
    os += "}";
    return os;
}

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
    EXPECT_TRUE(parseStringAsJson(makeDeepArray(10)))
        << "10 level deep should parse";

    EXPECT_FALSE(parseStringAsJson(makeDeepArray(11)))
        << "11 level deep should fail to parse";
}

TEST(HttpParsing, parseRequestAsJsonLimitsObjectDepths)
{
    EXPECT_TRUE(parseStringAsJson(makeDeepObject(9)))
        << "10 level deep should parse";
    EXPECT_FALSE(parseStringAsJson(makeDeepObject(10)))
        << "10 level deep should fail to parse";
}

TEST(HttpParsing, parseStringAsJsonMaxValues)
{
    EXPECT_TRUE(parseStringAsJson(makeWideArray(499)))
        << "499 values should parse";
    EXPECT_FALSE(parseStringAsJson(makeWideArray(501)))
        << "500 values should be rejected";

    // Keys and values are each counted separately, so 500/2 = 250 dict elements
    EXPECT_TRUE(parseStringAsJson(makeWideObject(249)))
        << "249 keys objects should parse";
    EXPECT_FALSE(parseStringAsJson(makeWideObject(250)))
        << "250 keys should be rejected";
}

} // namespace
