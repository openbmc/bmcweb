
#include "async_resp.hpp"
#include "http_request.hpp"
#include "redfish.hpp"
#include "utility.hpp"
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
    // Callback handler that does nothing
    bool oemCalled = false;
    auto oemCallback = [&oemCalled](const crow::Request&,
                                    const std::shared_ptr<bmcweb::AsyncResp>&,
                                    const std::string& bar) {
        oemCalled = true;
        EXPECT_EQ(bar, "bar");
    };
    bool standardCalled = false;
    auto standardCallback = [&standardCalled](const crow::Request&,
                                    const std::shared_ptr<bmcweb::AsyncResp>&,
                                    const std::string& bar) {
        standardCalled = true;
        EXPECT_EQ(bar, "bar");
    };

    std::error_code ec;
    App app;
    RedfishService service(app);
    app.validate();
    service.validate();
    // Need the normal route registered for OEM to work
    BMCWEB_ROUTE(app, "/foo/<str>")(standardCallback);
    REDFISH_SUB_ROUTE<"/foo/<str>#Oem">(service, HttpVerb::Get)(oemCallback);

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
