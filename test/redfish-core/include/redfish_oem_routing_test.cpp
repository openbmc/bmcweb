
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

TEST(OemRouter, PatchHandlerWithJsonObject)
{
    // Callback to test PATCH handler
    bool patchCalled1 = false;
    auto patchCallback1 =
        [&patchCalled1](
            const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
            const nlohmann::json::object_t& payload, const std::string& bar) {
            nlohmann::json jsonObject(payload);

            patchCalled1 = true;
            EXPECT_EQ(bar, "bar");
            auto it = payload.find("OemFooKey");
            ASSERT_NE(it, payload.end());
            EXPECT_EQ(it->second, "fooValue");
        };

    bool patchCalled2 = false;
    auto patchCallback2 =
        [&patchCalled2](
            const crow::Request&, const std::shared_ptr<bmcweb::AsyncResp>&,
            const nlohmann::json::object_t& payload, const std::string& bar) {
            nlohmann::json jsonObject(payload);

            patchCalled2 = true;
            EXPECT_EQ(bar, "bar");
            auto it = payload.find("OemBarKey");
            ASSERT_NE(it, payload.end());
            EXPECT_EQ(it->second, "barValue");
        };

    OemRouter router;
    std::error_code ec;
    constexpr std::string_view fragment1 = "/foo/<str>#Oem/OemFoo";
    router
        .newOemRule<crow::utility::getParameterTag(fragment1)>(
            std::string(fragment1))
        .setPatchHandler(patchCallback1);
    constexpr std::string_view fragment2 = "/foo/<str>#Oem/OemBar";
    router
        .newOemRule<crow::utility::getParameterTag(fragment2)>(
            std::string(fragment2))
        .setPatchHandler(patchCallback2);

    router.validate();
    {
        constexpr std::string_view reqUrl = "/foo/bar";

        auto req = std::make_shared<crow::Request>(
            crow::Request::Body{boost::beast::http::verb::get, reqUrl, 11}, ec);

        std::shared_ptr<bmcweb::AsyncResp> asyncResp =
            std::make_shared<bmcweb::AsyncResp>();

        nlohmann::json::object_t jsonPayload;
        jsonPayload["OemFoo"] = {{"OemFooKey", "fooValue"}};
        jsonPayload["OemBar"] = {{"OemBarKey", "barValue"}};
        router.handleOemPatch(req, asyncResp, jsonPayload);
    }
    EXPECT_TRUE(patchCalled1);
    EXPECT_TRUE(patchCalled2);
}

} // namespace
} // namespace redfish
