#include "async_resp.hpp"
#include "bios.hpp"
#include "http_request.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(PopulateSettings, StaticAttributesAreExpected)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    populateSettings(response);
    ASSERT_EQ("#Settings.v1_3_5.Settings",
              response->res.jsonValue["@Redfish.Settings"]["@odata.type"]);
    std::vector<std::string> supportedApplyTimes =
        response->res.jsonValue["@Redfish.Settings"]["SupportedApplyTimes"];
    EXPECT_THAT(std::vector<std::string>({"OnReset"}), supportedApplyTimes);
    ASSERT_EQ("/redfish/v1/Systems/system/Bios/Settings",
              response->res.jsonValue["@Redfish.Settings"]["SettingsObject"]
                                     ["@odata.id"]);
}

TEST(AddAttribute, ExpectedAttributes)
{
    const auto* testStringName = "After Power Loss";
    const auto* testStringValue = "Previous State";

    const auto* testNumberName = "BIOS Power-On Hour";
    auto testNumberValue = 5;

    nlohmann::json attributeList;
    addAttribute(attributeList, testStringName, testStringValue);
    addAttribute(attributeList, testNumberName, testNumberValue);

    ASSERT_EQ(testStringValue,
              attributeList[testStringName].get<std::string>());
    ASSERT_EQ(testNumberValue, attributeList[testNumberName].get<int64_t>());
}

TEST(PopulateRedfishFromBaseTable, ExpectedAttributes)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    BaseTable baseTable;
    baseTable["After Power Loss"] = BaseTableAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration",
        false, "After Power Loss", "", "", "Power Off", "Previous State",
        std::vector<BaseTableOption>(
            {{"xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf",
              "Power Off", "Power Off"},
             {"xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf",
              "Power On", "Power On"},
             {"xyz.openbmc_project.BIOSConfig.Manager.BoundType.OneOf",
              "Previous State", "Previous State"}}));

    baseTable["BIOS Power-On Hour"] = BaseTableAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer", false,
        "BIOS Power-On Hour", "", "", 5, 0, std::vector<BaseTableOption>());

    baseTable["IPv4 Address"] = BaseTableAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String", false,
        "IPv4 Address", "", "", "192.168.1.10", "",
        std::vector<BaseTableOption>());

    populateRedfishFromBaseTable(response, baseTable);

    auto attributeList = response->res.jsonValue["Attributes"];
    ASSERT_EQ("Power Off",
              attributeList["After Power Loss"].get<std::string>());
    ASSERT_EQ(5, attributeList["BIOS Power-On Hour"].get<int64_t>());
    ASSERT_EQ("192.168.1.10", attributeList["IPv4 Address"].get<std::string>());
}

TEST(PopulateRedfishFromPending, ExpectedAttributes)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    PendingAttributes pendingAttributes;
    pendingAttributes["After Power Loss"] = PendingAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration",
        "Power On");

    pendingAttributes["BIOS Power-On Hour"] = PendingAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer", 3);

    pendingAttributes["IPv4 Address"] = PendingAttribute(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
        "192.168.1.12");

    populateRedfishFromPending(response, pendingAttributes);

    auto attributeList = response->res.jsonValue["Attributes"];
    ASSERT_EQ("Power On", attributeList["After Power Loss"].get<std::string>());
    ASSERT_EQ(3, attributeList["BIOS Power-On Hour"].get<int64_t>());
    ASSERT_EQ("192.168.1.12", attributeList["IPv4 Address"].get<std::string>());
}

TEST(PopulatePendingFromRedfish, ValidData)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request putReq(R"({
        "Attributes": {
            "After Power Loss": "Power On",
            "BIOS Power-On Hour": 3,
            "IPv4 Address": "192.168.1.12"
        }
    })",
                         err);
    putReq.req.set(boost::beast::http::field::content_type, "application/json");
    PendingAttributes pendingAttributes;

    auto status =
        populatePendingFromRedfish(pendingAttributes, putReq, response);
    ASSERT_TRUE(status);
    ASSERT_EQ(3, pendingAttributes.size());
    ASSERT_EQ("Power On",
              std::get<std::string>(std::get<PendingAttributeIndex_Value>(
                  pendingAttributes["After Power Loss"])));
    ASSERT_EQ(3, std::get<int64_t>(std::get<PendingAttributeIndex_Value>(
                     pendingAttributes["BIOS Power-On Hour"])));
    ASSERT_EQ("192.168.1.12",
              std::get<std::string>(std::get<PendingAttributeIndex_Value>(
                  pendingAttributes["IPv4 Address"])));

    crow::Request patchReq(R"({
        "Attributes": {
            "After Power Loss": "Power Off",
            "Secure Boot": "Disable"
        }
    })",
                           err);
    patchReq.req.set(boost::beast::http::field::content_type,
                     "application/json");

    status = populatePendingFromRedfish(pendingAttributes, patchReq, response);
    ASSERT_TRUE(status);
    ASSERT_EQ(4, pendingAttributes.size());
    ASSERT_EQ("Power Off",
              std::get<std::string>(std::get<PendingAttributeIndex_Value>(
                  pendingAttributes["After Power Loss"])));
    ASSERT_EQ(3, std::get<int64_t>(std::get<PendingAttributeIndex_Value>(
                     pendingAttributes["BIOS Power-On Hour"])));
    ASSERT_EQ("192.168.1.12",
              std::get<std::string>(std::get<PendingAttributeIndex_Value>(
                  pendingAttributes["IPv4 Address"])));
    ASSERT_EQ("Disable",
              std::get<std::string>(std::get<PendingAttributeIndex_Value>(
                  pendingAttributes["Secure Boot"])));
}

TEST(PopulatePendingFromRedfish, BadType)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request putReq(R"({
        "Attributes": {
            "After Power Loss": {"Bad": "Type"}
        }
    })",
                         err);
    putReq.req.set(boost::beast::http::field::content_type, "application/json");
    PendingAttributes pendingAttributes;

    auto status =
        populatePendingFromRedfish(pendingAttributes, putReq, response);
    ASSERT_FALSE(status);
    ASSERT_EQ(boost::beast::http::status::bad_request, response->res.result());
}

TEST(PopulatePendingFromRedfish, InvalidJson)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::error_code err;
    crow::Request putReq(R"("Bad Data")", err);
    PendingAttributes pendingAttributes;

    auto status =
        populatePendingFromRedfish(pendingAttributes, putReq, response);
    ASSERT_FALSE(status);
    ASSERT_EQ(boost::beast::http::status::bad_request, response->res.result());
}

TEST(HandleBIOSMagangerDbusError, WithErrorOnly)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    response->res.jsonValue["Dummy"] = "Dummy";
    handleBIOSMagangerDbusError(response);
    ASSERT_EQ(boost::beast::http::status::internal_server_error,
              response->res.result());
    ASSERT_EQ(1, response->res.jsonValue.size());
    ASSERT_FALSE(response->res.jsonValue.contains("Dummy"));
    ASSERT_TRUE(response->res.jsonValue.contains("error"));
}

} // namespace
} // namespace redfish
