#include "app.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/service_root.hpp"

#include <boost/shared_ptr.hpp>

#include <iostream>
#include <string>

#include "gmock/gmock.h"

using namespace redfish;

class biosTest
{
  public:
    crow::Request* m_req;
    const std::shared_ptr<bmcweb::AsyncResp>* m_asyncRespPtr;

    void assertBiosPost()
    {
        EXPECT_EQ(json["@odata.id"], "/redfish/v1/Systems/system/Bios");
	EXPECT_EQ(json["@odata.type"], "#Bios.v1_1_0.Bios");
        EXPECT_EQ(json["Name"], "BIOS Configuration");
        EXPECT_EQ(json["Description"], "BIOS Configuration Service");
        EXPECT_EQ(json["Id"], "BIOS");
        nlohmann::json example;
        example["Actions"]["#Bios.ResetBios"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}}
        EXPECT_EQ(json["Actions"]["#Bios.ResetBios"],
               example["Actions"]["#Bios.ResetBios"]);

        // separate unit for fw_util::populateFirmwareInformation
    }
    BiosTest()
    {
        // create arguments used in handler call
        auto in =
            new boost::beast::http::request<boost::beast::http::string_body>;
        m_req = new crow::Request(*in);
        auto response = new crow::Response([this] { assertServiceRootGet(); });
        m_asyncRespPtr = new std::shared_ptr<bmcweb::AsyncResp>(
            new bmcweb::AsyncResp(*response));
        // call handler
        handleServiceRootGet(*m_req, *m_asyncRespPtr);
        (*m_asyncRespPtr)->res.end();
        // delete in;
        // delete m_req;
        // delete response;
        // delete m_asyncRespPtr;
    }
};

TEST(BiosTest, BiosConstructor)
{
    Bios srTest;
}


