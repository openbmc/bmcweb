#include "bios.hpp"
#include "http_response.hpp"

#include <sys/types.h>

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <string>
#include <vector>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(PopulateSettings, StaticAttributesAreExpected)
{
    crow::Response response;
    populateSettings(response);
    ASSERT_EQ("#Settings.v1_3_5.Settings",
              response.jsonValue["@Redfish.Settings"]["@odata.type"]);
    std::vector<std::string> supportedApplyTimes =
        response.jsonValue["@Redfish.Settings"]["SupportedApplyTimes"];
    EXPECT_THAT(std::vector<std::string>({"OnReset"}), supportedApplyTimes);
    ASSERT_EQ(
        "/redfish/v1/Systems/system/Bios/Settings",
        response.jsonValue["@Redfish.Settings"]["SettingsObject"]["@odata.id"]);
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
    crow::Response response;
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

    auto attributeList = response.jsonValue["Attributes"];
    ASSERT_EQ("Power Off",
              attributeList["After Power Loss"].get<std::string>());
    ASSERT_EQ(5, attributeList["BIOS Power-On Hour"].get<int64_t>());
    ASSERT_EQ("192.168.1.10", attributeList["IPv4 Address"].get<std::string>());
}

TEST(PopulateRedfishFromPending, ExpectedAttributes)
{
    crow::Response response;
    PendingAttributes pendingAttributes;
    pendingAttributes["After Power Loss"] = PendingAttributeValue(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Enumeration",
        std::string("Power On"));

    pendingAttributes["BIOS Power-On Hour"] = PendingAttributeValue(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.Integer", 3);

    pendingAttributes["IPv4 Address"] = PendingAttributeValue(
        "xyz.openbmc_project.BIOSConfig.Manager.AttributeType.String",
        "192.168.1.12");

    populateRedfishFromPending(response, pendingAttributes);

    auto attributeList = response.jsonValue["Attributes"];
    ASSERT_EQ("Power On", attributeList["After Power Loss"].get<std::string>());
    ASSERT_EQ(3, attributeList["BIOS Power-On Hour"].get<int64_t>());
    ASSERT_EQ("192.168.1.12", attributeList["IPv4 Address"].get<std::string>());
}

TEST(PopulatePendingFromRedfish, ValidData)
{
    crow::Response response;
    nlohmann::json putJsonAttributes;
    addAttribute(putJsonAttributes, "After Power Loss", "Power On");
    addAttribute(putJsonAttributes, "BIOS Power-On Hour", 3);
    addAttribute(putJsonAttributes, "IPv4 Address", "192.168.1.12");
    PendingAttributes pendingAttributes;
    auto status = populatePendingFromRedfish(pendingAttributes,
                                             putJsonAttributes, response);
    ASSERT_TRUE(status);
    ASSERT_EQ(3, pendingAttributes.size());
    ASSERT_EQ("Power On", std::get<std::string>(
                              std::get<uint(PendingAttributeValueIndex::Value)>(
                                  pendingAttributes["After Power Loss"])));
    ASSERT_EQ(
        3, std::get<int64_t>(std::get<uint(PendingAttributeValueIndex::Value)>(
               pendingAttributes["BIOS Power-On Hour"])));
    ASSERT_EQ(
        "192.168.1.12",
        std::get<std::string>(std::get<uint(PendingAttributeValueIndex::Value)>(
            pendingAttributes["IPv4 Address"])));

    nlohmann::json patchJsonAttributes;
    addAttribute(patchJsonAttributes, "After Power Loss", "Power Off");
    addAttribute(patchJsonAttributes, "Secure Boot", "Disable");

    status = populatePendingFromRedfish(pendingAttributes, patchJsonAttributes,
                                        response);
    ASSERT_TRUE(status);
    ASSERT_EQ(4, pendingAttributes.size());
    ASSERT_EQ(
        "Power Off",
        std::get<std::string>(std::get<uint(PendingAttributeValueIndex::Value)>(
            pendingAttributes["After Power Loss"])));
    ASSERT_EQ(
        3, std::get<int64_t>(std::get<uint(PendingAttributeValueIndex::Value)>(
               pendingAttributes["BIOS Power-On Hour"])));
    ASSERT_EQ(
        "192.168.1.12",
        std::get<std::string>(std::get<uint(PendingAttributeValueIndex::Value)>(
            pendingAttributes["IPv4 Address"])));
    ASSERT_EQ("Disable", std::get<std::string>(
                             std::get<uint(PendingAttributeValueIndex::Value)>(
                                 pendingAttributes["Secure Boot"])));
}

TEST(PopulatePendingFromRedfish, BadType)
{
    crow::Response response;
    nlohmann::json putJsonAttributes;
    putJsonAttributes["After Power Loss"] = nlohmann::json({"Bad", "Type"});
    PendingAttributes pendingAttributes;
    auto status = populatePendingFromRedfish(pendingAttributes,
                                             putJsonAttributes, response);
    ASSERT_FALSE(status);
    ASSERT_EQ(boost::beast::http::status::bad_request, response.result());
}

} // namespace
} // namespace redfish
