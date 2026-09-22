// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "generated/enums/computer_system.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"
#include "systems.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

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

TEST(Systems, DbusToRfBootType)
{
    EXPECT_EQ(
        dbusToRfBootType("xyz.openbmc_project.Control.Boot.Type.Types.Legacy"),
        computer_system::BootSourceOverrideMode::Legacy);

    EXPECT_EQ(
        dbusToRfBootType("xyz.openbmc_project.Control.Boot.Type.Types.EFI"),
        computer_system::BootSourceOverrideMode::UEFI);

    EXPECT_EQ(dbusToRfBootType("invalid"),
              computer_system::BootSourceOverrideMode::Invalid);
}

TEST(Systems, DbusToRfBootMode)
{
    EXPECT_EQ(
        dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"),
        computer_system::BootSource::None);

    EXPECT_EQ(
        dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Safe"),
        computer_system::BootSource::Diags);

    EXPECT_EQ(
        dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Setup"),
        computer_system::BootSource::BiosSetup);

    EXPECT_EQ(dbusToRfBootMode("invalid"),
              computer_system::BootSource::Invalid);
}

TEST(Systems, DbusToRfBootSource)
{
    EXPECT_EQ(dbusToRfBootSource(
                  "xyz.openbmc_project.Control.Boot.Source.Sources.Default"),
              "None");

    EXPECT_EQ(dbusToRfBootSource(
                  "xyz.openbmc_project.Control.Boot.Source.Sources.Disk"),
              "Hdd");

    EXPECT_EQ(
        dbusToRfBootSource(
            "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia"),
        "Cd");

    EXPECT_EQ(dbusToRfBootSource(
                  "xyz.openbmc_project.Control.Boot.Source.Sources.Network"),
              "Pxe");

    EXPECT_EQ(
        dbusToRfBootSource(
            "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia"),
        "Usb");

    EXPECT_EQ(dbusToRfBootSource("invalid"), "");
}

TEST(Systems, DbusToRfBootProgress)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified"),
        computer_system::BootProgressTypes::None);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PrimaryProcInit"),
        computer_system::BootProgressTypes::
            PrimaryProcessorInitializationStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.BusInit"),
        computer_system::BootProgressTypes::BusInitializationStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.MemoryInit"),
        computer_system::BootProgressTypes::MemoryInitializationStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SecondaryProcInit"),
        computer_system::BootProgressTypes::
            SecondaryProcessorInitializationStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PCIInit"),
        computer_system::BootProgressTypes::PCIResourceConfigStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SystemSetup"),
        computer_system::BootProgressTypes::SetupEntered);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SystemInitComplete"),
        computer_system::BootProgressTypes::
            SystemHardwareInitializationComplete);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart"),
        computer_system::BootProgressTypes::OSBootStarted);

    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSRunning"),
        computer_system::BootProgressTypes::OSRunning);

    EXPECT_EQ(dbusToRfBootProgress("invalid"),
              computer_system::BootProgressTypes::Invalid);
}

TEST(Systems, AssignBootParameters)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("None", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");

    EXPECT_EQ(assignBootParameters("Pxe", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Network");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");

    EXPECT_EQ(assignBootParameters("Hdd", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Disk");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");

    EXPECT_EQ(assignBootParameters("Cd", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");

    EXPECT_EQ(assignBootParameters("Usb", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");

    // Diags and BiosSetup select a boot mode and leave the source at default
    EXPECT_EQ(assignBootParameters("Diags", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Safe");

    EXPECT_EQ(assignBootParameters("BiosSetup", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup");

    EXPECT_EQ(assignBootParameters("invalid", bootSource, bootMode), -1);
}

TEST(Systems, RedfishPowerRestorePolicyFromDbus)
{
    EXPECT_EQ(
        redfishPowerRestorePolicyFromDbus(
            "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn"),
        computer_system::PowerRestorePolicyTypes::AlwaysOn);

    EXPECT_EQ(
        redfishPowerRestorePolicyFromDbus(
            "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff"),
        computer_system::PowerRestorePolicyTypes::AlwaysOff);

    EXPECT_EQ(
        redfishPowerRestorePolicyFromDbus(
            "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore"),
        computer_system::PowerRestorePolicyTypes::LastState);

    // Redfish has no equivalent of Policy.None, so it reports as AlwaysOff
    EXPECT_EQ(
        redfishPowerRestorePolicyFromDbus(
            "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.None"),
        computer_system::PowerRestorePolicyTypes::AlwaysOff);

    EXPECT_EQ(redfishPowerRestorePolicyFromDbus("invalid"),
              computer_system::PowerRestorePolicyTypes::Invalid);
}

TEST(Systems, DbusPowerRestorePolicyFromRedfish)
{
    EXPECT_EQ(
        dbusPowerRestorePolicyFromRedfish("AlwaysOn"),
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOn");

    EXPECT_EQ(
        dbusPowerRestorePolicyFromRedfish("AlwaysOff"),
        "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.AlwaysOff");

    EXPECT_EQ(dbusPowerRestorePolicyFromRedfish("LastState"),
              "xyz.openbmc_project.Control.Power.RestorePolicy.Policy.Restore");

    EXPECT_EQ(dbusPowerRestorePolicyFromRedfish("invalid"), "");
}

TEST(Systems, TranslatePowerModeString)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode.Static"),
              computer_system::PowerMode::Static);

    EXPECT_EQ(
        translatePowerModeString(
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.MaximumPerformance"),
        computer_system::PowerMode::MaximumPerformance);

    EXPECT_EQ(
        translatePowerModeString(
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.PowerSaving"),
        computer_system::PowerMode::PowerSaving);

    EXPECT_EQ(
        translatePowerModeString(
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.BalancedPerformance"),
        computer_system::PowerMode::BalancedPerformance);

    EXPECT_EQ(
        translatePowerModeString(
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPerformance"),
        computer_system::PowerMode::EfficiencyFavorPerformance);

    EXPECT_EQ(
        translatePowerModeString(
            "xyz.openbmc_project.Control.Power.Mode.PowerMode.EfficiencyFavorPower"),
        computer_system::PowerMode::EfficiencyFavorPower);

    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode.OEM"),
              computer_system::PowerMode::OEM);

    EXPECT_EQ(translatePowerModeString("invalid"),
              computer_system::PowerMode::Invalid);
}

TEST(Systems, DbusToRfWatchdogAction)
{
    EXPECT_EQ(dbusToRfWatchdogAction(
                  "xyz.openbmc_project.State.Watchdog.Action.None"),
              "None");

    EXPECT_EQ(dbusToRfWatchdogAction(
                  "xyz.openbmc_project.State.Watchdog.Action.HardReset"),
              "ResetSystem");

    EXPECT_EQ(dbusToRfWatchdogAction(
                  "xyz.openbmc_project.State.Watchdog.Action.PowerOff"),
              "PowerDown");

    EXPECT_EQ(dbusToRfWatchdogAction(
                  "xyz.openbmc_project.State.Watchdog.Action.PowerCycle"),
              "PowerCycle");

    EXPECT_EQ(dbusToRfWatchdogAction("invalid"), "");
}

TEST(Systems, RfToDbusWDTTimeOutAct)
{
    EXPECT_EQ(rfToDbusWDTTimeOutAct("None"),
              "xyz.openbmc_project.State.Watchdog.Action.None");

    EXPECT_EQ(rfToDbusWDTTimeOutAct("ResetSystem"),
              "xyz.openbmc_project.State.Watchdog.Action.HardReset");

    EXPECT_EQ(rfToDbusWDTTimeOutAct("PowerDown"),
              "xyz.openbmc_project.State.Watchdog.Action.PowerOff");

    EXPECT_EQ(rfToDbusWDTTimeOutAct("PowerCycle"),
              "xyz.openbmc_project.State.Watchdog.Action.PowerCycle");

    EXPECT_EQ(rfToDbusWDTTimeOutAct("invalid"), "");
}

TEST(Systems, WatchdogActionRoundTrip)
{
    for (const char* action :
         {"None", "ResetSystem", "PowerDown", "PowerCycle"})
    {
        EXPECT_EQ(dbusToRfWatchdogAction(rfToDbusWDTTimeOutAct(action)),
                  action);
    }
}

TEST(Systems, ValidStopBootOnFault)
{
    EXPECT_EQ(validstopBootOnFault("AnyFault"), std::make_optional(true));
    EXPECT_EQ(validstopBootOnFault("Never"), std::make_optional(false));
    EXPECT_EQ(validstopBootOnFault("invalid"), std::nullopt);
}

TEST(GetAllowedHostTransition, UnexpectedError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;
    std::vector<std::string> allowedHostTransitions;

    afterGetAllowedHostTransitions(response, ec, allowedHostTransitions);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(GetAllowedHostTransition, NoPropOnDbus)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;
    std::vector<std::string> allowedHostTransitions;

    afterGetAllowedHostTransitions(response, ec, allowedHostTransitions);

    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.emplace_back(resource::ResetType::ForceOff);
    allowed.emplace_back(resource::ResetType::PowerCycle);
    allowed.emplace_back(resource::ResetType::Nmi);
    allowed.emplace_back(resource::ResetType::On);
    allowed.emplace_back(resource::ResetType::ForceOn);
    allowed.emplace_back(resource::ResetType::ForceRestart);
    allowed.emplace_back(resource::ResetType::GracefulRestart);
    allowed.emplace_back(resource::ResetType::GracefulShutdown);
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    EXPECT_EQ(response->res.jsonValue["Parameters"], parameters);
}

TEST(GetAllowedHostTransition, NoForceRestart)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    std::vector<std::string> allowedHostTransitions = {
        "xyz.openbmc_project.State.Host.Transition.On",
        "xyz.openbmc_project.State.Host.Transition.Off",
        "xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot",
    };

    afterGetAllowedHostTransitions(response, ec, allowedHostTransitions);

    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.emplace_back(resource::ResetType::ForceOff);
    allowed.emplace_back(resource::ResetType::PowerCycle);
    allowed.emplace_back(resource::ResetType::Nmi);
    allowed.emplace_back(resource::ResetType::On);
    allowed.emplace_back(resource::ResetType::ForceOn);
    allowed.emplace_back(resource::ResetType::GracefulShutdown);
    allowed.emplace_back(resource::ResetType::GracefulRestart);
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    EXPECT_EQ(response->res.jsonValue["Parameters"], parameters);
}

TEST(GetAllowedHostTransition, AllSupported)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    std::vector<std::string> allowedHostTransitions = {
        "xyz.openbmc_project.State.Host.Transition.On",
        "xyz.openbmc_project.State.Host.Transition.Off",
        "xyz.openbmc_project.State.Host.Transition.GracefulWarmReboot",
        "xyz.openbmc_project.State.Host.Transition.ForceWarmReboot",
    };

    afterGetAllowedHostTransitions(response, ec, allowedHostTransitions);

    nlohmann::json::array_t parameters;
    nlohmann::json::object_t parameter;
    parameter["Name"] = "ResetType";
    parameter["Required"] = true;
    parameter["DataType"] = "String";
    nlohmann::json::array_t allowed;
    allowed.emplace_back(resource::ResetType::ForceOff);
    allowed.emplace_back(resource::ResetType::PowerCycle);
    allowed.emplace_back(resource::ResetType::Nmi);
    allowed.emplace_back(resource::ResetType::On);
    allowed.emplace_back(resource::ResetType::ForceOn);
    allowed.emplace_back(resource::ResetType::GracefulShutdown);
    allowed.emplace_back(resource::ResetType::GracefulRestart);
    allowed.emplace_back(resource::ResetType::ForceRestart);
    parameter["AllowableValues"] = std::move(allowed);
    parameters.emplace_back(std::move(parameter));

    EXPECT_EQ(response->res.jsonValue["Parameters"], parameters);
}

} // namespace
} // namespace redfish
