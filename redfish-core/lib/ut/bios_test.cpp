#include "app.hpp"
#include "bios.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <iostream> //cerr
#include <string>

#include "gmock/gmock.h"

void assertBiosGet(crow::Response& res)
{
    std::cerr << "starting assertBioGet " << &res << std::endl;
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
    std::cerr << "done with assert" << std::endl;
}

TEST(BiosTest, BiosConstructor)
{

    std::cerr << "starting test " << std::endl;
    boost::beast::http::request<boost::beast::http::string_body> in;
    std::error_code ec;
    crow::Request req(in, ec);
    const std::shared_ptr<bmcweb::AsyncResp> shareAsyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    std::cerr << "setting Req handler" << std::endl;
    shareAsyncResp->res.setCompleteRequestHandler(assertBiosGet);
    //  std::function < void(crow::Response&)> assertBiosGet);

    std::cerr << "biosServieGet" << std::endl;
    redfish::handleBiosServiceGet(req, shareAsyncResp);

    std::cerr << "outofScope for req handler call" << std::endl;
}
