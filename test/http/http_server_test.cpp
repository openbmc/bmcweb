#include "http/http_server.hpp"

#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::MatchesRegex;

namespace crow
{

TEST(HttpServer, getCachedDateStr)
{
    struct FakeHandler
    {};
    FakeHandler handler;
    crow::Server<FakeHandler> server(&handler, {});
    std::string firstString = server.getCachedDateStrImpl();
    EXPECT_THAT(
        firstString,
        MatchesRegex(
            "(Mon|Tue|Wed|Thu|Fri|Sat|Sun), (0[1-9]|[12][0-9]|3[01]) "
            "(Jan|Feb|Mar|Apr|May|Jun|Jul|Aug|Sep|Oct|Nov|Dec) [0-9]{4} "
            "([01][0-9]|2[0-3]):[0-5][0-9]:[0-5][0-9] GMT"));

    EXPECT_EQ(firstString, server.getCachedDateStrImpl());
}
} // namespace crow
