#include "app.hpp"
#include "bios.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <string>

#include "gmock/gmock.h"

void assertBiosGet(crow::Response& res)
{
    const nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/Systems/system/Bios");
    EXPECT_EQ(json["@odata.type"], "#Bios.v1_1_0.Bios");
    EXPECT_EQ(json["Name"], "BIOS Configuration");
    EXPECT_EQ(json["Description"], "BIOS Configuration Service");
    EXPECT_EQ(json["Id"], "BIOS");
    EXPECT_EQ(json["Actions"]["#Bios.ResetBios"]["target"],
              "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios");

    // separate unit for fw_util::populateFirmwareInformation
    EXPECT_EQ(28, json.size());
}

TEST(BiosTest, BiosConstructor)
{
    boost::beast::http::request<boost::beast::http::string_body> in;
    std::error_code ec;
    crow::Request req(in, ec);
    const std::shared_ptr<bmcweb::AsyncResp> shareAsyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    shareAsyncResp->res.setCompleteRequestHandler(assertBiosGet);

    redfish::handleBiosServiceGet(req, shareAsyncResp);
}
