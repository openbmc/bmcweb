#include "app.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/service_root.hpp"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <string>

#include "gmock/gmock.h"

void assertServiceRootGet(crow::Response& res)
{
    nlohmann::json json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/redfish/v1");
    EXPECT_EQ(json["@odata.type"], "#ServiceRoot.v1_5_0.ServiceRoot");

    EXPECT_EQ(json["AccountService"]["@odata.id"],
              "/redfish/v1/AccountService");
    EXPECT_EQ(json["CertificateService"]["@odata.id"],
              "/redfish/v1/CertificateService");
    EXPECT_EQ(json["Chassis"]["@odata.id"], "/redfish/v1/Chassis");
    EXPECT_EQ(json["EventService"]["@odata.id"], "/redfish/v1/EventService");
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

    std::string uuid = json["UUID"];
    std::vector<std::string> vectuuid;
    auto pos = uuid.find('-');
    while (pos != std::string::npos)
    {
        vectuuid.push_back(uuid.substr(0, pos));
        uuid = uuid.substr(pos + 1, std::string::npos);
        pos = uuid.find('-');
    }
    vectuuid.push_back(uuid.substr(0, std::string::npos));

    EXPECT_EQ(vectuuid.size(), 5);
    EXPECT_EQ(vectuuid[0].size(), 8);
    EXPECT_EQ(vectuuid[1].size(), 4);
    EXPECT_EQ(vectuuid[2].size(), 4);
    EXPECT_EQ(vectuuid[3].size(), 4);
    EXPECT_EQ(vectuuid[4].size(), 12);

    std::stringstream ss;
    unsigned long x;
    ss << std::hex << vectuuid[0];
    ss >> x;
    EXPECT_TRUE(x <= 0xffffffff);

    ss << std::hex << vectuuid[1];
    x = 0;
    ss >> x;
    EXPECT_TRUE(x <= 0xffff);

    ss << std::hex << vectuuid[2];
    x = 0;
    ss >> x;
    EXPECT_TRUE(x <= 0xffff);

    ss << std::hex << vectuuid[3];
    x = 0;
    ss >> x;
    EXPECT_TRUE(x <= 0xffff);

    ss << std::hex << vectuuid[4];
    x = 0;
    ss >> x;
    EXPECT_TRUE(x <= 0xffffffffffff);

    EXPECT_EQ(json["UpdateService"]["@odata.id"], "/redfish/v1/UpdateService");
    EXPECT_EQ(19, json.size());
}

TEST(ServiceRootTest, ServiceRootConstructor)
{

    boost::beast::http::request<boost::beast::http::string_body> in;
    std::error_code ec;
    crow::Request req(in, ec);
    const std::shared_ptr<bmcweb::AsyncResp> shareAsyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    redfish::handleServiceRootGet(req, shareAsyncResp);

    shareAsyncResp->res.setCompleteRequestHandler(
        [](crow::Response& inRes) mutable { assertServiceRootGet(inRes); });
}
