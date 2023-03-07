#include "async_resp.hpp"
#include "google/google_service_root.hpp"
#include "http_request.hpp"
#include "nlohmann/json.hpp"

#include <gtest/gtest.h>

namespace crow::google_api
{
namespace
{

void validateServiceRootGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/google/v1");
    EXPECT_EQ(json["@odata.type"],
              "#GoogleServiceRoot.v1_0_0.GoogleServiceRoot");
    EXPECT_EQ(json["@odata.id"], "/google/v1");
    EXPECT_EQ(json["Id"], "Google Rest RootService");
    EXPECT_EQ(json["Name"], "Google Service Root");
    EXPECT_EQ(json["Version"], "1.0.0");
    EXPECT_EQ(json["RootOfTrustCollection"]["@odata.id"],
              "/google/v1/RootOfTrustCollection");
}

TEST(HandleGoogleV1Get, OnSuccess)
{
    App app;
    std::error_code ec;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    asyncResp->res.setCompleteRequestHandler(validateServiceRootGet);

    crow::Request dummyRequest{{boost::beast::http::verb::get, "", 11}, ec};
    handleGoogleV1Get(app, dummyRequest, asyncResp);
}

} // namespace
} // namespace crow::google_api
