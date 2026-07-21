// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
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

TEST(dbusToRfBootProgress, Unspecified)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unspecified"),
        "None");
}

TEST(dbusToRfBootProgress, PrimaryProcInit)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PrimaryProcInit"),
        "PrimaryProcessorInitializationStarted");
}

TEST(dbusToRfBootProgress, BusInit)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.BusInit"),
        "BusInitializationStarted");
}

TEST(dbusToRfBootProgress, MemoryInit)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.MemoryInit"),
        "MemoryInitializationStarted");
}

TEST(dbusToRfBootProgress, SecondaryProcInit)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SecondaryProcInit"),
        "SecondaryProcessorInitializationStarted");
}

TEST(dbusToRfBootProgress, PCIInit)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.PCIInit"),
        "PCIResourceConfigStarted");
}

TEST(dbusToRfBootProgress, SystemSetup)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SystemSetup"),
        "SetupEntered");
}

TEST(dbusToRfBootProgress, SystemInitComplete)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.SystemInitComplete"),
        "SystemHardwareInitializationComplete");
}

TEST(dbusToRfBootProgress, OSStart)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSStart"),
        "OSBootStarted");
}

TEST(dbusToRfBootProgress, OSRunning)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.OSRunning"),
        "OSRunning");
}

TEST(dbusToRfBootProgress, Unknown)
{
    EXPECT_EQ(
        dbusToRfBootProgress(
            "xyz.openbmc_project.State.Boot.Progress.ProgressStages.Unknown"),
        "None");
}

TEST(assignBootParameters, None)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("None", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

TEST(assignBootParameters, Pxe)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Pxe", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Network");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

TEST(assignBootParameters, Hdd)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Hdd", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Disk");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

TEST(assignBootParameters, Diags)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Diags", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Safe");
}

TEST(assignBootParameters, Cd)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Cd", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

TEST(assignBootParameters, BiosSetup)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("BiosSetup", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Setup");
}

TEST(assignBootParameters, Usb)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Usb", bootSource, bootMode), 0);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

TEST(assignBootParameters, Invalid)
{
    std::string bootSource;
    std::string bootMode;

    EXPECT_EQ(assignBootParameters("Invalid", bootSource, bootMode), -1);
    EXPECT_EQ(bootSource,
              "xyz.openbmc_project.Control.Boot.Source.Sources.Default");
    EXPECT_EQ(bootMode, "xyz.openbmc_project.Control.Boot.Mode.Modes.Regular");
}

} // namespace
} // namespace redfish
