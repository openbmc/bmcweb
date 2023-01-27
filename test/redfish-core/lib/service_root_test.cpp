#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "http_response.hpp"
#include "nlohmann/json.hpp"
#include "service_root.hpp"

#include <memory>
#include <vector>

#include <gmock/gmock.h> // IWYU pragma: keep
#include <gtest/gtest.h> // IWYU pragma: keep

// IWYU pragma: no_include <gtest/gtest-message.h>
// IWYU pragma: no_include <gtest/gtest-test-part.h>
// IWYU pragma: no_include "gtest/gtest_pred_impl.h"
// IWYU pragma: no_include <gmock/gmock-matchers.h>
// IWYU pragma: no_include <gtest/gtest-matchers.h>

namespace redfish
{
namespace
{

void assertServiceRootGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/redfish/v1");
    EXPECT_EQ(json["@odata.type"], "#ServiceRoot.v1_15_0.ServiceRoot");

    EXPECT_EQ(json["AccountService"]["@odata.id"],
              "/redfish/v1/AccountService");
    EXPECT_EQ(json["AccountService"].size(), 1);

    EXPECT_EQ(json["CertificateService"]["@odata.id"],
              "/redfish/v1/CertificateService");
    EXPECT_EQ(json["CertificateService"].size(), 1);

    EXPECT_EQ(json["Chassis"]["@odata.id"], "/redfish/v1/Chassis");
    EXPECT_EQ(json["Chassis"].size(), 1);

    EXPECT_EQ(json["EventService"]["@odata.id"], "/redfish/v1/EventService");
    EXPECT_EQ(json["EventService"].size(), 1);

    EXPECT_EQ(json["Id"], "RootService");
    EXPECT_EQ(json["Links"]["Sessions"]["@odata.id"],
              "/redfish/v1/SessionService/Sessions");
    EXPECT_EQ(json["Links"].size(), 2);
    EXPECT_EQ(json["Links"]["Sessions"].size(), 1);
    EXPECT_EQ(json["Links"]["ManagerProvidingService"].size(), 1);
    EXPECT_EQ(json["Links"]["ManagerProvidingService"]["@odata.id"],
              "/redfish/v1/Managers/bmc");

    EXPECT_EQ(json["Managers"]["@odata.id"], "/redfish/v1/Managers");
    EXPECT_EQ(json["Managers"].size(), 1);

    EXPECT_EQ(json["Name"], "Root Service");
    EXPECT_EQ(json["RedfishVersion"], "1.9.0");

    EXPECT_EQ(json["Registries"]["@odata.id"], "/redfish/v1/Registries");
    EXPECT_EQ(json["Registries"].size(), 1);

    EXPECT_EQ(json["SessionService"]["@odata.id"],
              "/redfish/v1/SessionService");
    EXPECT_EQ(json["SessionService"].size(), 1);

    EXPECT_EQ(json["Systems"]["@odata.id"], "/redfish/v1/Systems");
    EXPECT_EQ(json["Systems"].size(), 1);

    EXPECT_EQ(json["Tasks"]["@odata.id"], "/redfish/v1/TaskService");
    EXPECT_EQ(json["Tasks"].size(), 1);

    EXPECT_EQ(json["TelemetryService"]["@odata.id"],
              "/redfish/v1/TelemetryService");
    EXPECT_EQ(json["TelemetryService"].size(), 1);

    EXPECT_THAT(
        json["UUID"],
        testing::MatchesRegex("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-"
                              "9a-fA-F]{4}-[0-9a-fA-F]{12}"));

    EXPECT_EQ(json["UpdateService"]["@odata.id"], "/redfish/v1/UpdateService");

    EXPECT_EQ(json["ProtocolFeaturesSupported"].size(), 6);
    EXPECT_FALSE(json["ProtocolFeaturesSupported"]["ExcerptQuery"]);
    EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"]["ExpandAll"],
              bmcwebInsecureEnableQueryParams);
    EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"]["Levels"],
              bmcwebInsecureEnableQueryParams);
    EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"]["Links"],
              bmcwebInsecureEnableQueryParams);
    EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"]["NoLinks"],
              bmcwebInsecureEnableQueryParams);
    if (bmcwebInsecureEnableQueryParams)
    {
        EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"].size(), 5);
        EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"]["MaxLevels"],
                  6);
    }
    else
    {
        EXPECT_EQ(json["ProtocolFeaturesSupported"]["ExpandQuery"].size(), 4);
    }
    EXPECT_FALSE(json["ProtocolFeaturesSupported"]["FilterQuery"]);
    EXPECT_TRUE(json["ProtocolFeaturesSupported"]["OnlyMemberQuery"]);
    EXPECT_TRUE(json["ProtocolFeaturesSupported"]["SelectQuery"]);
    EXPECT_FALSE(
        json["ProtocolFeaturesSupported"]["DeepOperations"]["DeepPOST"]);
    EXPECT_FALSE(
        json["ProtocolFeaturesSupported"]["DeepOperations"]["DeepPATCH"]);
    EXPECT_EQ(json["ProtocolFeaturesSupported"]["DeepOperations"].size(), 2);
    EXPECT_EQ(json.size(), 21);
}

TEST(HandleServiceRootGet, ServiceRootStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();

    shareAsyncResp->res.setCompleteRequestHandler(assertServiceRootGet);

    redfish::handleServiceRootGetImpl(shareAsyncResp);
}

} // namespace
} // namespace redfish
