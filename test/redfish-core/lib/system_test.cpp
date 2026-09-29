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

TEST(Systems, DbusToRfBootProgress)
{
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "Unspecified"),
              computer_system::BootProgressTypes::None);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "PrimaryProcInit"),
              computer_system::BootProgressTypes::
                  PrimaryProcessorInitializationStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "BusInit"),
              computer_system::BootProgressTypes::BusInitializationStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "MemoryInit"),
              computer_system::BootProgressTypes::MemoryInitializationStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "SecondaryProcInit"),
              computer_system::BootProgressTypes::
                  SecondaryProcessorInitializationStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "PCIInit"),
              computer_system::BootProgressTypes::PCIResourceConfigStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "SystemSetup"),
              computer_system::BootProgressTypes::SetupEntered);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "SystemInitComplete"),
              computer_system::BootProgressTypes::
                  SystemHardwareInitializationComplete);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "OSStart"),
              computer_system::BootProgressTypes::OSBootStarted);
    EXPECT_EQ(dbusToRfBootProgress(
                  "xyz.openbmc_project.State.Boot.Progress.ProgressStages."
                  "OSRunning"),
              computer_system::BootProgressTypes::OSRunning);
    EXPECT_EQ(dbusToRfBootProgress("invalid"),
              computer_system::BootProgressTypes::Invalid);
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
