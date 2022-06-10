#include "app.hpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace crow
{
namespace
{

using ::testing::ElementsAre;
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
    BMCWEB_ROUTE(app, "/abc")([]() { return boost::beast::http::status::ok; });
    app.validate();

    EXPECT_THAT(app.getRoutes(), ElementsAre(Pointee(Eq("/abc"))));
}

// Tests that static urls are correctly passed
TEST(GetRoutes, TestlotsOfRoutes)
{
    App app;

    BMCWEB_ROUTE(app, "/a/1")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/foo")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/bar")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/baz")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/boo")([]() { return boost::beast::http::status::ok; });
    BMCWEB_ROUTE(app, "/moo")([]() { return boost::beast::http::status::ok; });
    app.validate();

    EXPECT_THAT(app.getRoutes(),
                UnorderedElementsAre(Pointee(Eq("/a/1")), Pointee(Eq("/foo")),
                                     Pointee(Eq("/bar")), Pointee(Eq("/baz")),
                                     Pointee(Eq("/boo")), Pointee(Eq("/moo"))));
}
} // namespace
} // namespace crow