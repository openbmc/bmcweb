// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_response.hpp"
#include "processor.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetProcessorLocationCode, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorLocationCode(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorLocationCode, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorLocationCode(response, ec, "U3-P0");

    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"],
        "U3-P0");
}

TEST(DbusToRfThrottleCause, ValidCausesTranslatedCorrectly)
{
    EXPECT_EQ(dbusToRfThrottleCause(
                  "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
                  "ClockLimit"),
              processor::ThrottleCause::ClockLimit);
    EXPECT_EQ(dbusToRfThrottleCause(
                  "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
                  "ManagementDetectedFault"),
              processor::ThrottleCause::ManagementDetectedFault);
    EXPECT_EQ(dbusToRfThrottleCause(
                  "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
                  "PowerLimit"),
              processor::ThrottleCause::PowerLimit);
    EXPECT_EQ(dbusToRfThrottleCause(
                  "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
                  "ThermalLimit"),
              processor::ThrottleCause::ThermalLimit);
    EXPECT_EQ(dbusToRfThrottleCause(
                  "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
                  "Unknown"),
              processor::ThrottleCause::Unknown);
}

TEST(DbusToRfThrottleCause, UnrecognizedCauseReturnsInvalid)
{
    EXPECT_EQ(dbusToRfThrottleCause("unrecognized.cause"),
              processor::ThrottleCause::Invalid);
    EXPECT_EQ(dbusToRfThrottleCause(""), processor::ThrottleCause::Invalid);
}

TEST(ReadThrottleProperties, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;
    dbus::utility::DBusPropertiesMap properties;

    readThrottleProperties(response, ec, properties);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(ReadThrottleProperties, InvalidCauseSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    bool throttled = true;
    std::vector<std::string> causes = {"invalid.throttle.cause"};

    dbus::utility::DBusPropertiesMap properties = {{"Throttled", throttled},
                                                   {"ThrottleCauses", causes}};

    readThrottleProperties(response, ec, properties);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(ReadThrottleProperties, ValidDataPopulatesResponse)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    bool throttled = true;
    std::vector<std::string> causes = {
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons.PowerLimit",
        "xyz.openbmc_project.Control.Power.Throttle.ThrottleReasons."
        "ThermalLimit"};

    dbus::utility::DBusPropertiesMap properties = {{"Throttled", throttled},
                                                   {"ThrottleCauses", causes}};

    readThrottleProperties(response, ec, properties);

    EXPECT_EQ(response->res.jsonValue["Throttled"], true);
    nlohmann::json::array_t expectedCauses = {
        processor::ThrottleCause::PowerLimit,
        processor::ThrottleCause::ThermalLimit};
    EXPECT_EQ(response->res.jsonValue["ThrottleCauses"], expectedCauses);
}

TEST(AfterGetCpuAssetData, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;
    dbus::utility::DBusPropertiesMap properties;

    afterGetCpuAssetData(response, ec, properties);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetCpuAssetData, IntelManufacturerSetsX86Architecture)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::string manufacturer = "Intel Corporation";

    dbus::utility::DBusPropertiesMap properties = {
        {"Manufacturer", manufacturer}};

    afterGetCpuAssetData(response, ec, properties);

    EXPECT_EQ(response->res.jsonValue["Manufacturer"], "Intel Corporation");
    EXPECT_EQ(response->res.jsonValue["ProcessorArchitecture"], "x86");
    EXPECT_EQ(response->res.jsonValue["InstructionSet"], "x86-64");
}

TEST(AfterGetCpuAssetData, IbmManufacturerSetsPowerArchitecture)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::string manufacturer = "IBM";

    dbus::utility::DBusPropertiesMap properties = {
        {"Manufacturer", manufacturer}};

    afterGetCpuAssetData(response, ec, properties);

    EXPECT_EQ(response->res.jsonValue["Manufacturer"], "IBM");
    EXPECT_EQ(response->res.jsonValue["ProcessorArchitecture"], "Power");
    EXPECT_EQ(response->res.jsonValue["InstructionSet"], "PowerISA");
}

TEST(AfterGetCpuAssetData, AmpereManufacturerSetsArmArchitecture)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::string manufacturer = "Ampere Computing";

    dbus::utility::DBusPropertiesMap properties = {
        {"Manufacturer", manufacturer}};

    afterGetCpuAssetData(response, ec, properties);

    EXPECT_EQ(response->res.jsonValue["Manufacturer"], "Ampere Computing");
    EXPECT_EQ(response->res.jsonValue["ProcessorArchitecture"], "ARM");
    EXPECT_EQ(response->res.jsonValue["InstructionSet"], "ARM-A64");
}

TEST(AfterGetCpuAssetData, UnknownManufacturerNoArchitecture)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::string manufacturer = "Unknown Vendor";

    dbus::utility::DBusPropertiesMap properties = {
        {"Manufacturer", manufacturer}};

    afterGetCpuAssetData(response, ec, properties);

    EXPECT_EQ(response->res.jsonValue["Manufacturer"], "Unknown Vendor");
    EXPECT_FALSE(response->res.jsonValue.contains("ProcessorArchitecture"));
    EXPECT_FALSE(response->res.jsonValue.contains("InstructionSet"));
}

} // namespace
} // namespace redfish
