#include "app.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/service_root.hpp"

#include <boost/shared_ptr.hpp>

#include <string>

#include "gmock/gmock.h"


void assertServiceRootGet(crow::Response res)
{
        nlohmann::json json = (*shareAsyncResp)->res.jsonValue;
        EXPECT_EQ(json["@odata.id"], "/redfish/v1");
        EXPECT_EQ(json["@odata.type"], "#ServiceRoot.v1_5_0.ServiceRoot");

        EXPECT_EQ(json["AccountService"]["@odata.id"],
                  "/redfish/v1/AccountService");
        EXPECT_EQ(json["CertificateService"]["@odata.id"],
                  "/redfish/v1/CertificateService");
        EXPECT_EQ(json["Chassis"]["@odata.id"], "/redfish/v1/Chassis");
        EXPECT_EQ(json["EventService"]["@odata.id"],
                  "/redfish/v1/EventService");
        EXPECT_EQ(json["Id"], "RootService");
        EXPECT_EQ(json["Links"]["Sessions"]["@odata.id"],
                  "/redfish/v1/SessionService/Sessions");
        EXPECT_EQ(json["Managers"]["@odata.id"], "/redfish/v1/Managers");
        EXPECT_EQ(json["Name"], "Root Service");
        EXPECT_EQ(json["RedfishVersion"], "1.9.0");
        EXPECT_EQ(json["Name"], "Root Service");
        EXPECT_EQ(json["Registries"]["@odata.id"], "/redfish/v1/Registries");
        EXPECT_EQ(json["SessionService"]["@odata.id"],
                  "/redfish/v1/SessionService");
        EXPECT_EQ(json["Systems"]["@odata.id"], "/redfish/v1/Systems");
        EXPECT_EQ(json["Tasks"]["@odata.id"], "/redfish/v1/TaskService");

        EXPECT_EQ(json["TelemetryService"]["@odata.id"],
                  "/redfish/v1/TelemetryService");

        // TODO write UUDI checking logic
        // EXPECT_EQ(json["UUID"], "");
        EXPECT_EQ(json["UpdateService"]["@odata.id"],
                  "/redfish/v1/UpdateService");
        // EXPECT_EQ(28,json.size());
}

TEST(ServiceRootTest, ServiceRootConstructor)
{
    boost::beast::http::request<boost::beast::http::string_body> in;
    crow::Response response;
    crow::Request req(std::move(in));
    crow::Response response;
    response.setCompleteRequestHandler( assertServiceRootGet(req));
    std::shared_ptr<bmcweb::AsyncResp> shareAsyncRespTemp =
    std::make_shared<bmcweb::AsyncResp>(response);
    shareAsyncResp = &shareAsyncRespTemp;
    redfish::handleServiceRootGet(req, *shareAsyncResp);
}
