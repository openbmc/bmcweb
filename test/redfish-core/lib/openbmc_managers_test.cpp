// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "openbmc/openbmc_managers.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterAsyncPopulatePid, ErrorCodeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::host_unreachable;

    afterAsyncPopulatePid(asyncResp, "", {}, ec, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, EmptyManagedObjectsPopulatesStaticFields)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterAsyncPopulatePid(asyncResp, "", {"ProfileA", "ProfileB"}, {}, {});

    const nlohmann::json& fan = asyncResp->res.jsonValue["Fan"];
    EXPECT_EQ(fan["@odata.type"], "#OpenBMCManager.v1_1_0.Manager.Fan");
    EXPECT_EQ(fan["@odata.id"], "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan");
    EXPECT_EQ(fan["FanControllers"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.FanControllers");
    EXPECT_EQ(fan["PidControllers"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.PidControllers");
    EXPECT_EQ(fan["StepwiseControllers"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.StepwiseControllers");
    EXPECT_EQ(fan["FanZones"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.FanZones");

    std::vector<std::string> profiles =
        fan["Profile@Redfish.AllowableValues"].get<std::vector<std::string>>();
    EXPECT_EQ(profiles, (std::vector<std::string>{"ProfileA", "ProfileB"}));

    // No "Profile" key when currentProfile is empty.
    EXPECT_FALSE(fan.contains("Profile"));
}

TEST(AfterAsyncPopulatePid, CurrentProfileSetsProfileField)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterAsyncPopulatePid(asyncResp, "ProfileA", {"ProfileA"}, {}, {});

    EXPECT_EQ(asyncResp->res.jsonValue["Fan"]["Profile"], "ProfileA");
}

TEST(AfterAsyncPopulatePid, FanControllerEntryIsPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyFan");
    props.emplace_back("Class", "fan");
    props.emplace_back("PCoefficient", 1.5);
    props.emplace_back("OutLimitMax", 100.0);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/fan0"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& fans =
        asyncResp->res.jsonValue["Fan"]["FanControllers"];
    ASSERT_TRUE(fans.contains("MyFan"));
    EXPECT_EQ(fans["MyFan"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.FanController");
    EXPECT_EQ(fans["MyFan"]["@odata.id"],
              "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanControllers/MyFan");
    EXPECT_DOUBLE_EQ(fans["MyFan"]["PCoefficient"].get<double>(), 1.5);
    EXPECT_DOUBLE_EQ(fans["MyFan"]["OutLimitMax"].get<double>(), 100.0);
    EXPECT_FALSE(
        asyncResp->res.jsonValue["Fan"]["PidControllers"].contains("MyFan"));
}

TEST(AfterAsyncPopulatePid, PidControllerEntryIsPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    props.emplace_back("Class", "temperature");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/pid0"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& pids =
        asyncResp->res.jsonValue["Fan"]["PidControllers"];
    ASSERT_TRUE(pids.contains("MyPid"));
    EXPECT_EQ(pids["MyPid"]["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.PidController");
    EXPECT_EQ(pids["MyPid"]["@odata.id"],
              "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/PidControllers/MyPid");
}

TEST(AfterAsyncPopulatePid, StepwiseControllerStepsAssembled)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyStep");
    props.emplace_back("Class", "Ceiling");
    props.emplace_back("Reading", std::vector<double>{20.0, 30.0, 40.0});
    props.emplace_back("Output", std::vector<double>{10.0, 50.0, 80.0});
    props.emplace_back("PositiveHysteresis", 1.0);
    props.emplace_back("NegativeHysteresis", 2.0);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Stepwise",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/step0"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& step =
        asyncResp->res.jsonValue["Fan"]["StepwiseControllers"]["MyStep"];
    EXPECT_EQ(step["@odata.type"],
              "#OpenBMCManager.v1_1_0.Manager.StepwiseController");
    EXPECT_EQ(step["Direction"], "Ceiling");
    EXPECT_DOUBLE_EQ(step["PositiveHysteresis"].get<double>(), 1.0);
    EXPECT_DOUBLE_EQ(step["NegativeHysteresis"].get<double>(), 2.0);

    const nlohmann::json& steps = step["Steps"];
    ASSERT_TRUE(steps.is_array());
    ASSERT_EQ(steps.size(), 3U);
    EXPECT_DOUBLE_EQ(steps[0]["Target"].get<double>(), 20.0);
    EXPECT_DOUBLE_EQ(steps[0]["Output"].get<double>(), 10.0);
    EXPECT_DOUBLE_EQ(steps[2]["Target"].get<double>(), 40.0);
    EXPECT_DOUBLE_EQ(steps[2]["Output"].get<double>(), 80.0);
}

TEST(AfterAsyncPopulatePid, FanZoneIsPopulatedWithDoubles)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "Zone0");
    props.emplace_back("MinThermalOutput", 25.0);
    props.emplace_back("FailSafePercent", 75.0);

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid.Zone",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(
        sdbusplus::message::object_path("/xyz/chassis/MyChassis"),
        std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& zone =
        asyncResp->res.jsonValue["Fan"]["FanZones"]["Zone0"];
    EXPECT_EQ(zone["@odata.type"], "#OpenBMCManager.v1_1_0.Manager.FanZone");
    EXPECT_EQ(zone["Chassis"]["@odata.id"], "/redfish/v1/Chassis/MyChassis");
    EXPECT_EQ(zone["@odata.id"],
              "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Zone0");
    EXPECT_DOUBLE_EQ(zone["MinThermalOutput"].get<double>(), 25.0);
    EXPECT_DOUBLE_EQ(zone["FailSafePercent"].get<double>(), 75.0);
}

TEST(AfterAsyncPopulatePid, FanZoneIllegalChassisGetsPlaceholder)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "Zone0");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid.Zone",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    // empty filename portion -> chassis becomes "#IllegalValue"
    managed.emplace_back(sdbusplus::message::object_path("/"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& zone =
        asyncResp->res.jsonValue["Fan"]["FanZones"]["Zone0"];
    // The '#' must be percent-encoded in the path segment.
    EXPECT_EQ(zone["Chassis"]["@odata.id"],
              "/redfish/v1/Chassis/%23IllegalValue");
}

TEST(AfterAsyncPopulatePid, UnknownInterfacesAreIgnored)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "Other");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.SomethingElse",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/x"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(
        asyncResp->res.jsonValue["Fan"]["PidControllers"].contains("Other"));
    EXPECT_FALSE(
        asyncResp->res.jsonValue["Fan"]["FanControllers"].contains("Other"));
}

TEST(AfterAsyncPopulatePid, BadNameTypeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", 42);
    props.emplace_back("Class", "fan");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, BadProfilesTypeReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    props.emplace_back("Profiles", "not-a-list");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "ProfileA", {"ProfileA"}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, MissingClassOnPidReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    // No "Class" property at all.

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, MissingClassOnStepwiseReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyStep");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Stepwise",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/s"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, SetPointOffsetTranslatesKnownValues)
{
    struct Translation
    {
        std::string dbus;
        std::string redfish;
    };

    const auto translations = std::to_array<Translation>(
        {{"WarningHigh", "UpperThresholdNonCritical"},
         {"WarningLow", "LowerThresholdNonCritical"},
         {"CriticalHigh", "UpperThresholdCritical"},
         {"CriticalLow", "LowerThresholdCritical"}});

    for (const auto& t : translations)
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

        dbus::utility::DBusPropertiesMap props;
        props.emplace_back("Name", "MyPid");
        props.emplace_back("Class", "temperature");
        props.emplace_back("SetPointOffset", t.dbus);

        dbus::utility::DBusInterfacesMap interfaces;
        interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                                std::move(props));

        dbus::utility::ManagedObjectType managed;
        managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                             std::move(interfaces));

        afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

        EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok)
            << "dbus value: " << t.dbus;
        EXPECT_EQ(asyncResp->res.jsonValue["Fan"]["PidControllers"]["MyPid"]
                                          ["SetPointOffset"],
                  t.redfish);
    }
}

TEST(AfterAsyncPopulatePid, SetPointOffsetUnknownValueReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    props.emplace_back("Class", "temperature");
    props.emplace_back("SetPointOffset", "NotARealValue");

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, StepwiseReadingOutputSizeMismatchReturnsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyStep");
    props.emplace_back("Class", "Ceiling");
    props.emplace_back("Reading", std::vector<double>{1.0, 2.0});
    props.emplace_back("Output", std::vector<double>{10.0, 20.0, 30.0});

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Stepwise",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/s"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterAsyncPopulatePid, PidZonesReferenceUsesFanZonesUrl)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    props.emplace_back("Class", "temperature");
    props.emplace_back("Zones", std::vector<std::string>{"Zone0", "Zone1"});

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& zones =
        asyncResp->res.jsonValue["Fan"]["PidControllers"]["MyPid"]["Zones"];
    ASSERT_TRUE(zones.is_array());
    ASSERT_EQ(zones.size(), 2U);
    EXPECT_EQ(zones[0]["@odata.id"],
              "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Zone0");
    EXPECT_EQ(zones[1]["@odata.id"],
              "/redfish/v1/Managers/bmc#/Oem/OpenBmc/Fan/FanZones/Zone1");
}

TEST(AfterAsyncPopulatePid, InputsAndOutputsArePassedThrough)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Name", "MyPid");
    props.emplace_back("Class", "temperature");
    props.emplace_back("Inputs",
                       std::vector<std::string>{"Sensor1", "Sensor2"});
    props.emplace_back("Outputs", std::vector<std::string>{"FanOut1"});

    dbus::utility::DBusInterfacesMap interfaces;
    interfaces.emplace_back("xyz.openbmc_project.Configuration.Pid",
                            std::move(props));

    dbus::utility::ManagedObjectType managed;
    managed.emplace_back(sdbusplus::message::object_path("/xyz/p"),
                         std::move(interfaces));

    afterAsyncPopulatePid(asyncResp, "", {}, {}, managed);

    const nlohmann::json& pid =
        asyncResp->res.jsonValue["Fan"]["PidControllers"]["MyPid"];
    EXPECT_EQ(pid["Inputs"], (std::vector<std::string>{"Sensor1", "Sensor2"}));
    EXPECT_EQ(pid["Outputs"], (std::vector<std::string>{"FanOut1"}));
}

} // namespace
} // namespace redfish
