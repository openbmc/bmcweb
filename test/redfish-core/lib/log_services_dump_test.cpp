#include "app.hpp"
#include "event_service_manager.hpp"
#include "include/async_resp.hpp"
#include "redfish-core/lib/health.hpp"
#include "redfish-core/lib/log_services.hpp"

#include <nlohmann/json.hpp>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

void assertLogServicesDumpServiceGet(crow::Response& res)
{
    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.type"], "#LogService.v1_2_0.LogService");
    EXPECT_EQ(json["Name"], "Dump LogService");
}

void assertLogServicesBMCDumpServiceGet(crow::Response& res)
{
    assertLogServicesDumpServiceGet(res);

    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/Managers/bmc/LogServices/Dump");
    EXPECT_EQ(
        json["Actions"]["#LogService.ClearLog"]["target"],
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.ClearLog");
    EXPECT_EQ(
        json["Actions"]["#LogService.CollectDiagnosticData"]["target"],
        "/redfish/v1/Managers/bmc/LogServices/Dump/Actions/LogService.CollectDiagnosticData");
    EXPECT_EQ(json["Description"], "BMC Dump LogService");
    EXPECT_EQ(json["Entries"]["@odata.id"],
              "/redfish/v1/Managers/bmc/LogServices/Dump/Entries");
    EXPECT_EQ(json["Id"], "Dump");
    EXPECT_EQ(json["OverWritePolicy"], "WrapsWhenFull");
}

void assertLogServicesFaultLogDumpServiceGet(crow::Response& res)
{
    assertLogServicesDumpServiceGet(res);

    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"],
              "/redfish/v1/Managers/bmc/LogServices/FaultLog");
    EXPECT_EQ(
        json["Actions"]["#LogService.ClearLog"]["target"],
        "/redfish/v1/Managers/bmc/LogServices/FaultLog/Actions/LogService.ClearLog");
    EXPECT_EQ(json["Actions"]["#LogService.CollectDiagnosticData"]["target"],
              nlohmann::detail::value_t::null);
    EXPECT_EQ(json["Description"], "FaultLog Dump LogService");
    EXPECT_EQ(json["Entries"]["@odata.id"],
              "/redfish/v1/Managers/bmc/LogServices/FaultLog/Entries");
    EXPECT_EQ(json["Id"], "FaultLog");
    EXPECT_EQ(json["OverWritePolicy"], "Unknown");
}

void assertLogServicesSystemDumpServiceGet(crow::Response& res)
{
    assertLogServicesDumpServiceGet(res);

    nlohmann::json& json = res.jsonValue;
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/Systems/system/LogServices/Dump");
    EXPECT_EQ(
        json["Actions"]["#LogService.ClearLog"]["target"],
        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.ClearLog");
    EXPECT_EQ(
        json["Actions"]["#LogService.CollectDiagnosticData"]["target"],
        "/redfish/v1/Systems/system/LogServices/Dump/Actions/LogService.CollectDiagnosticData");
    EXPECT_EQ(json["Description"], "System Dump LogService");
    EXPECT_EQ(json["Entries"]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/Dump/Entries");
    EXPECT_EQ(json["Id"], "Dump");
    EXPECT_EQ(json["OverWritePolicy"], "WrapsWhenFull");
}

TEST(LogServicesDumpServiceTest,
     LogServicesBMCDumpServiceStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    shareAsyncResp->res.setCompleteRequestHandler(
        assertLogServicesBMCDumpServiceGet);
    getDumpServiceInfo(shareAsyncResp, "BMC");
}

TEST(LogServicesDumpServiceTest,
     LogServicesFaultLogDumpServiceStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    shareAsyncResp->res.setCompleteRequestHandler(
        assertLogServicesFaultLogDumpServiceGet);
    getDumpServiceInfo(shareAsyncResp, "FaultLog");
}

TEST(LogServicesDumpServiceTest,
     LogServicesSystemDumpServiceStaticAttributesAreExpected)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    shareAsyncResp->res.setCompleteRequestHandler(
        assertLogServicesSystemDumpServiceGet);
    getDumpServiceInfo(shareAsyncResp, "System");
}

TEST(LogServicesDumpServiceTest, LogServicesInvalidDumpServiceGetReturnsError)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    getDumpServiceInfo(shareAsyncResp, "Invalid");
    EXPECT_EQ(shareAsyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
