// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "http/parsing.hpp"

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

TEST(HttpParsing, parseRequestAsJson)
{
    std::string veryDeepArray;
    for (size_t i = 0; i < 10; i++)
    {
        veryDeepArray = "[" + veryDeepArray + "]";
    }

    EXPECT_TRUE(parseStringAsJson(veryDeepArray));

    veryDeepArray = "[" + veryDeepArray + "]";
    EXPECT_FALSE(parseStringAsJson(veryDeepArray));
}

} // namespace
