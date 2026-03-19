// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "fabric.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

constexpr const char* switchPath =
    "/xyz/openbmc_project/inventory/system/fabric0/switch0";
constexpr const char* chassisPath =
    "/xyz/openbmc_project/inventory/system/chassis/chassis0";

TEST(DbusToRfPowerState, MapsKnownValues)
{
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState.State.On"),
              resource::PowerState::On);
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState.State.Off"),
              resource::PowerState::Off);
    EXPECT_EQ(
        dbusToRfPowerState(
            "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn"),
        resource::PowerState::PoweringOn);
    EXPECT_EQ(dbusToRfPowerState(
                  "xyz.openbmc_project.State.Decorator.PowerState."
                  "State.PoweringOff"),
              resource::PowerState::PoweringOff);
}

TEST(DbusToRfPowerState, UnmappedValueReturnsNullopt)
{
    EXPECT_EQ(
        dbusToRfPowerState(
            "xyz.openbmc_project.State.Decorator.PowerState.State.Unknown"),
        std::nullopt);
    EXPECT_EQ(dbusToRfPowerState("On"), std::nullopt);
    EXPECT_EQ(dbusToRfPowerState(""), std::nullopt);
}

TEST(AfterGetSwitchPowerState, GenericErrorReportsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchPowerState(response, switchPath, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSwitchPowerState, AbsentPropertyOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchPowerState(response, switchPath, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerState, UnmappedValueOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchPowerState(
        response, switchPath, ec,
        "xyz.openbmc_project.State.Decorator.PowerState.State.Unknown");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerState, MappedValuesSetPowerState)
{
    constexpr std::array<std::pair<const char*, const char*>, 4> cases{
        {{"xyz.openbmc_project.State.Decorator.PowerState.State.On", "On"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.Off", "Off"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn",
          "PoweringOn"},
         {"xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOff",
          "PoweringOff"}}};

    for (const auto& [dbusValue, expected] : cases)
    {
        auto response = std::make_shared<bmcweb::AsyncResp>();
        boost::system::error_code ec;

        afterGetSwitchPowerState(response, switchPath, ec, dbusValue);

        EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
        ASSERT_TRUE(response->res.jsonValue.contains("PowerState"))
            << "dbus value: " << dbusValue;
        EXPECT_EQ(response->res.jsonValue["PowerState"], expected)
            << "dbus value: " << dbusValue;
    }
}

TEST(AfterGetSwitchPowerStateService, EbadrOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerStateService, IoErrorOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchPowerStateService, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSwitchPowerStateService, EmptyObjectOmitsPowerState)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchPowerStateService(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("PowerState"));
}

TEST(AfterGetSwitchContainedBy, AbsentAssociationOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchContainedBy(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchContainedBy, AnUnreachableMapperOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchContainedBy(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchContainedBy, NoChassisOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchContainedBy(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchContainedBy, MoreThanOneChassisIsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse paths{
        chassisPath, "/xyz/openbmc_project/inventory/system/chassis/chassis1"};

    afterGetSwitchContainedBy(response, ec, paths);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetSwitchContainedBy, OneChassisSetsTheLink)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreePathsResponse paths{chassisPath};

    afterGetSwitchContainedBy(response, ec, paths);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["Chassis"]["@odata.id"],
              "/redfish/v1/Chassis/chassis0");
}

TEST(AfterGetSwitchPCIeDevice, AbsentInterfaceOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetSwitchPCIeDevice(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchPCIeDevice, IoErrorOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetSwitchPCIeDevice(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchPCIeDevice, AnUnreachableMapperOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetSwitchPCIeDevice(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchPCIeDevice, ASwitchThatIsNotAPCIeDeviceOmitsLinks)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSwitchPCIeDevice(response, switchPath, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("Links"));
}

TEST(AfterGetSwitchPCIeDevice, APCIeDeviceSwitchSetsTheLink)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object;
    object.emplace_back("xyz.openbmc_project.Inventory.Manager",
                        std::vector<std::string>{
                            "xyz.openbmc_project.Inventory.Item.PCIeDevice"});

    afterGetSwitchPCIeDevice(response, switchPath, ec, object);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["Links"]["PCIeDevice"]["@odata.id"],
              "/redfish/v1/Systems/system/PCIeDevices/switch0");
}

} // namespace
} // namespace redfish
