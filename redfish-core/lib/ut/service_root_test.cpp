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

class ServiceRootTest
{
  public:
    crow::Request* m_req;
    const std::shared_ptr<bmcweb::AsyncResp>* m_asyncRespPtr;

    void assertServiceRootGet()
    {
        auto json = (*m_asyncRespPtr)->res.jsonValue;
        nlohmann::json example;
        EXPECT_EQ(json["@odata.id"], "/redfish/v1");
        EXPECT_EQ(json["@odata.type"], "#ServiceRoot.v1_5_0.ServiceRoot");

        example["AccountService"] = {
            {"@odata.id", "/redfish/v1/AccountService"}};
        EXPECT_EQ(json["AccountService"], example["AccountService"]);

        example["CertificateService"] = {
            {"@odata.id", "/redfish/v1/CertificateService"}};
        EXPECT_EQ(json["CertificateService"], example["CertificateService"]);

        example["Chassis"] = {{"@odata.id", "/redfish/v1/Chassis"}};
        EXPECT_EQ(json["Chassis"], example["Chassis"]);

        example["EventService"] = {{"@odata.id", "/redfish/v1/EventService"}};
        EXPECT_EQ(json["EventService"], example["EventService"]);

        EXPECT_EQ(json["Id"], "RootService");

        example["Links"]["Sessions"] = {
            {"@odata.id", "/redfish/v1/SessionService/Sessions"}};
        EXPECT_EQ(json["Links"]["Sessions"], example["Links"]["Sessions"]);

        example["Managers"] = {{"@odata.id", "/redfish/v1/Managers"}};
        EXPECT_EQ(json["Managers"], example["Managers"]);
        EXPECT_EQ(json["Name"], "Root Service");
        EXPECT_EQ(json["RedfishVersion"], "1.9.0");
        EXPECT_EQ(json["Name"], "Root Service");

        example["Registries"] = {{"@odata.id", "/redfish/v1/Registries"}};
        EXPECT_EQ(json["Registries"], example["Registries"]);

        example["SessionService"] = {
            {"@odata.id", "/redfish/v1/SessionService"}};
        EXPECT_EQ(json["SessionsService"], example["SessionsService"]);

        example["Systems"] = {{"@odata.id", "/redfish/v1/Systems"}};
        EXPECT_EQ(json["Systems"], example["Systems"]);

        example["Tasks"] = {{"@odata.id", "/redfish/v1/TaskService"}};
        EXPECT_EQ(json["Tasks"], example["Tasks"]);

        example["TelemetryService"] = {
            {"@odata.id", "/redfish/v1/TelemetryService"}};
        EXPECT_EQ(json["TelemetryService"], example["TelemetryService"]);

        // TODO write UUDI checking logic
        // EXPECT_EQ(json["UUID"], "");
        example["UpdateService"] = {{"@odata.id", "/redfish/v1/UpdateService"}};
        EXPECT_EQ(json["UpdateService"], example["UpdateService"]);

        std::cerr << "JEBR DONE\n";
        return;
    }

    ServiceRootTest()
    {
        std::cerr << "JEBR serviceRootTest\n";

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

TEST(ServiceRootTest, ServiceRootConstructor)
{
    ServiceRootTest srTest;
}
