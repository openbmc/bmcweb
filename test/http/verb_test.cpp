#include "verb.hpp"

#include <boost/beast/http/verb.hpp>

#include <map>
#include <optional>
#include <string_view>

#include <gtest/gtest.h> // IWYU pragma: keep

using BoostVerb = boost::beast::http::verb;

TEST(BoostToHttpVerb, ValidCase)
{
    std::map<HttpVerb, BoostVerb> verbMap = {
        {HttpVerb::Delete, BoostVerb::delete_},
        {HttpVerb::Get, BoostVerb::get},
        {HttpVerb::Head, BoostVerb::head},
        {HttpVerb::Options, BoostVerb::options},
        {HttpVerb::Patch, BoostVerb::patch},
        {HttpVerb::Post, BoostVerb::post},
        {HttpVerb::Put, BoostVerb::put},
    };

    for (int verb_index = 0; verb_index < static_cast<int>(HttpVerb::Max);
         ++verb_index)
    {
        HttpVerb http_verb = static_cast<HttpVerb>(verb_index);
        std::optional<HttpVerb> verb = httpVerbFromBoost(verbMap[http_verb]);
        EXPECT_TRUE(verb.has_value());
        EXPECT_EQ(*verb, http_verb);
    }
}

TEST(BoostToHttpVerbTest, InvalidCase)
{
    std::optional<HttpVerb> verb = httpVerbFromBoost(BoostVerb::unknown);
    EXPECT_FALSE(verb.has_value());
}

TEST(HttpVerbToStringTest, ValidCase)
{
    std::map<HttpVerb, std::string_view> verbMap = {
        {HttpVerb::Delete, "DELETE"}, {HttpVerb::Get, "GET"},
        {HttpVerb::Head, "HEAD"},     {HttpVerb::Options, "OPTIONS"},
        {HttpVerb::Patch, "PATCH"},   {HttpVerb::Post, "POST"},
        {HttpVerb::Put, "PUT"},
    };

    for (int verb_index = 0; verb_index < static_cast<int>(HttpVerb::Max);
         ++verb_index)
    {
        HttpVerb http_verb = static_cast<HttpVerb>(verb_index);
        EXPECT_EQ(httpVerbToString(http_verb), verbMap[http_verb]);
    }
}