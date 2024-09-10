// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"

#include <memory>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace crow
{
namespace
{

using ::bmcweb::AsyncResp;
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

    BMCWEB_ROUTE(app, "/")
    ([](const crow::Request& /*req*/,
        const std::shared_ptr<AsyncResp>& /*asyncResp*/) {});

    // TODO: "/" doesn't get reported in |getRoutes| today. Uncomment this once
    // it is fixed
    // EXPECT_THAT(app.getRoutes(),
    // testing::ElementsAre(Pointee(Eq("/"))));
}

// Tests that static urls are correctly passed
TEST(GetRoutes, TestlotsOfRoutes)
{
    App app;
    BMCWEB_ROUTE(app, "/")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});
    BMCWEB_ROUTE(app, "/foo")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});
    BMCWEB_ROUTE(app, "/bar")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});
    BMCWEB_ROUTE(app, "/baz")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});
    BMCWEB_ROUTE(app, "/boo")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});
    BMCWEB_ROUTE(app, "/moo")
    ([](const Request& /*req*/, const std::shared_ptr<AsyncResp>& /*res*/) {});

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
