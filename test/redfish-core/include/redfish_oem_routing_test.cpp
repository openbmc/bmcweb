
#include "async_resp.hpp"
#include "http_request.hpp"
#include "redfish_oem_routing.hpp"
#include "utility.hpp"

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
    // Callback handler that does nothing
    bool oemCalled = false;
    auto oemCallback = [&oemCalled](const crow::Request&,
                                    const std::shared_ptr<bmcweb::AsyncResp>&,
                                    const std::string& bar) {
        oemCalled = true;
        EXPECT_EQ(bar, "bar");
    };

    OemRouter router;
    std::error_code ec;
    constexpr std::string_view fragment = "/foo/<str>#Oem";
    router
        .newOemRule<crow::utility::getParameterTag(fragment)>(
            std::string(fragment))
        .setGetHandler(oemCallback);
    router.validate();
    {
        constexpr std::string_view reqUrl = "/foo/bar";

        auto req = std::make_shared<crow::Request>(
            crow::Request::Body{boost::beast::http::verb::get, reqUrl, 11}, ec);

        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        router.handleOemGet(req, asyncResp);
    }
    EXPECT_TRUE(oemCalled);
}

} // namespace
} // namespace redfish
