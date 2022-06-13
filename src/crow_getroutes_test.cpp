#include <app.hpp>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace crow;
using namespace std;

TEST(GetRoutes, TestEmptyRoutes)
{
    App app;
    app.validate();

    EXPECT_THAT(app.getRoutes(), testing::IsEmpty());
}

// Tests that static urls are correctly passed
TEST(GetRoutes, TestOneRoute)
{
    App app;

    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });

    // TODO: "/" doesn't get reported in |getRoutes| today. Uncomment this once
    // it is fixed
    // EXPECT_THAT(app.getRoutes(),
    // testing::ElementsAre(testing::Pointee(std::string("/"))));
}

// Tests that static urls are correctly passed
TEST(GetRoutes, TestlotsOfRoutes)
{
    App app;
    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/foo")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/bar")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/baz")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/boo")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/moo")([]() { return boost::beast::http::status::ok; });

    app.validate();

    // TODO: "/" doesn't get reported in |getRoutes| today. Uncomment this once
    // it is fixed
    EXPECT_THAT(app.getRoutes(), testing::UnorderedElementsAre(
                                     // testing::Pointee(std::string("/")),
                                     testing::Pointee(std::string("/foo")),
                                     testing::Pointee(std::string("/bar")),
                                     testing::Pointee(std::string("/baz")),
                                     testing::Pointee(std::string("/boo")),
                                     testing::Pointee(std::string("/moo"))));
}
