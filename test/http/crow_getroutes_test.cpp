#include "app.hpp"
#include "routing.hpp"

#include <boost/beast/http/status.hpp>

#include <memory>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gmock/gmock-matchers.h>
// IWYU pragma: no_include <gmock/gmock-more-matchers.h>
// IWYU pragma: no_include <gtest/gtest-matchers.h>

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
} // namespace
} // namespace crow
