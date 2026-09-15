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
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

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

TEST(DbusToRfBootProgress, UnspecifiedReturnsNone)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.Unspecified"),
              "None");
}

TEST(DbusToRfBootProgress,
     PrimaryProcInitReturnsPrimaryProcessorInitializationStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.PrimaryProcInit"),
              "PrimaryProcessorInitializationStarted");
}

TEST(DbusToRfBootProgress, BusInitReturnsBusInitializationStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.BusInit"),
              "BusInitializationStarted");
}

TEST(DbusToRfBootProgress, MemoryInitReturnsMemoryInitializationStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.MemoryInit"),
              "MemoryInitializationStarted");
}

TEST(DbusToRfBootProgress,
     SecondaryProcInitReturnsSecondaryProcessorInitializationStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.SecondaryProcInit"),
              "SecondaryProcessorInitializationStarted");
}

TEST(DbusToRfBootProgress, PciInitReturnsPciResourceConfigStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.PCIInit"),
              "PCIResourceConfigStarted");
}

TEST(DbusToRfBootProgress, SystemSetupReturnsSetupEntered)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.SystemSetup"),
              "SetupEntered");
}

TEST(DbusToRfBootProgress,
     SystemInitCompleteReturnsSystemHardwareInitializationComplete)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.SystemInitComplete"),
              "SystemHardwareInitializationComplete");
}

TEST(DbusToRfBootProgress, OsStartReturnsOsBootStarted)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.OSStart"),
              "OSBootStarted");
}

TEST(DbusToRfBootProgress, OsRunningReturnsOsRunning)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.OSRunning"),
              "OSRunning");
}

TEST(DbusToRfBootProgress, UnmappedProgressStageReturnsNone)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.MotherboardInit"),
              "None");
}

TEST(DbusToRfBootProgress, MatchIsCaseSensitive)
{
    EXPECT_EQ(dbusToRfBootProgress("xyz.openbmc_project.State.Boot.Progress."
                                   "ProgressStages.osrunning"),
              "None");
}

TEST(DbusToRfBootProgress, EmptyStringReturnsNone)
{
    EXPECT_EQ(dbusToRfBootProgress(""), "None");
}

TEST(DbusToRfBootProgress, RedfishValueWithoutDbusPrefixReturnsNone)
{
    EXPECT_EQ(dbusToRfBootProgress("OSRunning"), "None");
}

TEST(TranslatePowerModeString, Static)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "Static"),
              computer_system::PowerMode::Static);
}

TEST(TranslatePowerModeString, MaximumPerformance)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "MaximumPerformance"),
              computer_system::PowerMode::MaximumPerformance);
}

TEST(TranslatePowerModeString, PowerSaving)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "PowerSaving"),
              computer_system::PowerMode::PowerSaving);
}

TEST(TranslatePowerModeString, BalancedPerformance)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "BalancedPerformance"),
              computer_system::PowerMode::BalancedPerformance);
}

TEST(TranslatePowerModeString, EfficiencyFavorPerformance)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "EfficiencyFavorPerformance"),
              computer_system::PowerMode::EfficiencyFavorPerformance);
}

TEST(TranslatePowerModeString, EfficiencyFavorPower)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "EfficiencyFavorPower"),
              computer_system::PowerMode::EfficiencyFavorPower);
}

TEST(TranslatePowerModeString, Oem)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "OEM"),
              computer_system::PowerMode::OEM);
}

TEST(TranslatePowerModeString, OsControlledIsNotMapped)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "OSControlled"),
              computer_system::PowerMode::Invalid);
}

TEST(TranslatePowerModeString, MatchIsCaseSensitive)
{
    EXPECT_EQ(translatePowerModeString(
                  "xyz.openbmc_project.Control.Power.Mode.PowerMode."
                  "static"),
              computer_system::PowerMode::Invalid);
}

TEST(TranslatePowerModeString, EmptyStringIsInvalid)
{
    EXPECT_EQ(translatePowerModeString(""),
              computer_system::PowerMode::Invalid);
}

TEST(TranslatePowerModeString, RedfishValueWithoutDbusPrefixIsInvalid)
{
    EXPECT_EQ(translatePowerModeString("Static"),
              computer_system::PowerMode::Invalid);
}

} // namespace
} // namespace redfish
