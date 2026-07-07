// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "processor_metrics.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <limits>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetCoreVoltageProperties, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetCoreVoltageProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/voltage/Voltage_0",
        ec, 1.05);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetCoreVoltageProperties, NonFiniteReadingOmitsCoreVoltage)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetCoreVoltageProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/voltage/Voltage_0",
        ec, std::numeric_limits<double>::quiet_NaN());

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("CoreVoltage"));
}

TEST(AfterGetCoreVoltageProperties, SuccessSetsReadingAndDataSourceUri)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetCoreVoltageProperties(
        response, "chassis0", "/xyz/openbmc_project/sensors/voltage/Voltage_0",
        ec, 1.05);

    EXPECT_EQ(response->res.jsonValue["CoreVoltage"]["Reading"], 1.05);
    EXPECT_EQ(response->res.jsonValue["CoreVoltage"]["DataSourceUri"],
              "/redfish/v1/Chassis/chassis0/Sensors/voltage_Voltage_0");
}

TEST(HandleCoreVoltageSensors, EbadrOmitsCoreVoltage)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    handleCoreVoltageSensors(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("CoreVoltage"));
}

TEST(HandleCoreVoltageSensors, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    handleCoreVoltageSensors(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(HandleCoreVoltageSensors, EmptySubTreeOmitsCoreVoltage)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    handleCoreVoltageSensors(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("CoreVoltage"));
}

TEST(HandleCoreVoltageSensors, MultipleSensorsSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/sensors/voltage/Voltage_0",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Sensor.Value"}}}},
        {"/xyz/openbmc_project/sensors/voltage/Voltage_1",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Sensor.Value"}}}}};

    handleCoreVoltageSensors(response, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(HandleCoreVoltageSensors, EmptyServiceMapSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/sensors/voltage/Voltage_0", {}}};

    handleCoreVoltageSensors(response, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(HandleCoreVoltageSensors, MultipleServicesSetInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/sensors/voltage/Voltage_0",
         {{"xyz.openbmc_project.Service0",
           {"xyz.openbmc_project.Sensor.Value"}},
          {"xyz.openbmc_project.Service1",
           {"xyz.openbmc_project.Sensor.Value"}}}}};

    handleCoreVoltageSensors(response, ec, subtree);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
