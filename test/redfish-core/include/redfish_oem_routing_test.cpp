
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "redfish.hpp"
#include "verb.hpp"

#include <boost/beast/http/verb.hpp>

#include <memory>
#include <string>
#include <string_view>
#include <system_error>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(OemRouter, FragmentRoutes)
{
    std::error_code ec;
    App app;
    RedfishService service(app);

    // Callback handler that does nothing
    bool oemCalled = false;
    auto oemCallback = [&oemCalled](const crow::Request&,
                                    const std::shared_ptr<bmcweb::AsyncResp>&,
                                    const std::string& bar) {
        oemCalled = true;
        EXPECT_EQ(bar, "bar");
    };
    bool standardCalled = false;
    auto standardCallback =
        [&standardCalled,
         &service](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& bar) {
            service.handleSubRoute(req, asyncResp);
            standardCalled = true;
            EXPECT_EQ(bar, "bar");
        };
    // Need the normal route registered for OEM to work
    BMCWEB_ROUTE(app, "/foo/<str>/")
        .methods(boost::beast::http::verb::get)(standardCallback);
    REDFISH_SUB_ROUTE<"/foo/<str>/#/Oem">(service, HttpVerb::Get)(oemCallback);

    app.validate();
    service.validate();

    {
        constexpr std::string_view reqUrl = "/foo/bar";

        std::shared_ptr<crow::Request> req = std::make_shared<crow::Request>(
            crow::Request::Body{boost::beast::http::verb::get, reqUrl, 11}, ec);

        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        app.handle(req, asyncResp);
    }
    EXPECT_TRUE(oemCalled);
    EXPECT_TRUE(standardCalled);
}

} // namespace
} // namespace redfish
