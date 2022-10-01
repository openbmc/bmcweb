#include "verb.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/unordered/unordered_flat_map.hpp>

#include <optional>
#include <string_view>

#include <gtest/gtest.h> // IWYU pragma: keep

using BoostVerb = boost::beast::http::verb;

TEST(BoostToHttpVerb, ValidCase)
{
    boost::unordered_flat_map<HttpVerb, BoostVerb> verbMap = {
        {HttpVerb::Delete, BoostVerb::delete_},
        {HttpVerb::Get, BoostVerb::get},
        {HttpVerb::Head, BoostVerb::head},
        {HttpVerb::Options, BoostVerb::options},
        {HttpVerb::Patch, BoostVerb::patch},
        {HttpVerb::Post, BoostVerb::post},
        {HttpVerb::Put, BoostVerb::put},
    };

    for (int verbIndex = 0; verbIndex < static_cast<int>(HttpVerb::Max);
         ++verbIndex)
    {
        HttpVerb httpVerb = static_cast<HttpVerb>(verbIndex);
        std::optional<HttpVerb> verb = httpVerbFromBoost(verbMap[httpVerb]);
        EXPECT_EQ(verb, httpVerb);
    }
}

TEST(BoostToHttpVerbTest, InvalidCase)
{
    std::optional<HttpVerb> verb = httpVerbFromBoost(BoostVerb::unknown);
    EXPECT_FALSE(verb.has_value());
}

TEST(HttpVerbToStringTest, ValidCase)
{
    boost::unordered_flat_map<HttpVerb, std::string_view> verbMap = {
        {HttpVerb::Delete, "DELETE"}, {HttpVerb::Get, "GET"},
        {HttpVerb::Head, "HEAD"},     {HttpVerb::Options, "OPTIONS"},
        {HttpVerb::Patch, "PATCH"},   {HttpVerb::Post, "POST"},
        {HttpVerb::Put, "PUT"},
    };

    for (int verbIndex = 0; verbIndex < static_cast<int>(HttpVerb::Max);
         ++verbIndex)
    {
        HttpVerb httpVerb = static_cast<HttpVerb>(verbIndex);
        EXPECT_EQ(httpVerbToString(httpVerb), verbMap[httpVerb]);
    }
}
