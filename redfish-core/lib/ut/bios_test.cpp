#include "app.hpp"
#include "bios.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <string>

#include "gmock/gmock.h"

using namespace redfish;

class BiosTest
{
  public:
    crow::Request req;
    std::shared_ptr<bmcweb::AsyncResp>* shareAsyncResp;

    void assertBiosPost()
    {
        nlohmann::json json = (*shareAsyncResp)->res.jsonValue;
        EXPECT_EQ(json["@odata.id"], "/redfish/v1/Systems/system/Bios");
        EXPECT_EQ(json["@odata.type"], "#Bios.v1_1_0.Bios");
        EXPECT_EQ(json["Name"], "BIOS Configuration");
        EXPECT_EQ(json["Description"], "BIOS Configuration Service");
        EXPECT_EQ(json["Id"], "BIOS");
        nlohmann::json example;
        example["Actions"]["#Bios.ResetBios"] = {
            {"target",
             "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};
        // TODO before pushing, fix
        EXPECT_EQ(json["Actions"]["#Bios.ResetBios"],
                  example["Actions"]["#Bios.ResetBios"]);

        // separate unit for fw_util::populateFirmwareInformation
        EXPECT_EQ(28, json.size());
    }
    BiosTest(
        boost::beast::http::request<boost::beast::http::string_body> inReq) :
        req(inReq)
    {
        // create arguments used in handler call
        crow::Response response;
        response.setCompleteRequestHandler([this] { assertBiosPost(); });
        std::shared_ptr<bmcweb::AsyncResp> shareAsyncRespTemp =
            std::make_shared<bmcweb::AsyncResp>(response);
        shareAsyncResp = &shareAsyncRespTemp;
        // call handler
        redfish::handleBiosServiceGet(req, *shareAsyncResp);
    }
};

TEST(BiosTest, BiosConstructor)
{
    boost::beast::http::request<boost::beast::http::string_body> in;
    BiosTest biosTest(in);
}
