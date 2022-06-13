#include "app.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace crow
{
namespace
{

using ::testing::Eq;
using ::testing::IsEmpty;
using ::testing::Pointee;
using ::testing::UnorderedElementsAre;

TEST(GetRoutes, TestEmptyRoutes)
{
    App app;
    app.validate();

    EXPECT_THAT(app.getRoutes(), IsEmpty());
}

// Tests that static urls are correctly passed
TEST(GetRoutes, TestOneRoute)
{
    App app;

    BMCWEB_ROUTE(app, "/")([]() { return boost::beast::http::status::ok; });

    // TODO: "/" doesn't get reported in |getRoutes| today. Uncomment this once
    // it is fixed
    // EXPECT_THAT(app.getRoutes(),
    // testing::ElementsAre(Pointee(Eq("/"))));
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
    EXPECT_THAT(app.getRoutes(), UnorderedElementsAre(
                                     // Pointee(Eq("/")),
                                     Pointee(Eq("/foo")), Pointee(Eq("/bar")),
                                     Pointee(Eq("/baz")), Pointee(Eq("/boo")),
                                     Pointee(Eq("/moo"))));
}
}