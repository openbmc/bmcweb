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

TEST(dbusToRfBootSource, Default)
{
  EXPECT_EQ(dbusToRfBootSource("xyz.openbmc_project.Control.Boot.Source.Sources.Default"), "None");
}

TEST(dbusToRfBootSource, Disk)
{
  EXPECT_EQ(dbusToRfBootSource("xyz.openbmc_project.Control.Boot.Source.Sources.Disk"), "Hdd");
}

TEST(dbusToRfBootSource, ExternalMedia)
{
  EXPECT_EQ(dbusToRfBootSource("xyz.openbmc_project.Control.Boot.Source.Sources.ExternalMedia"), "Cd");
}

TEST(dbusToRfBootSource, Network)
{
  EXPECT_EQ(dbusToRfBootSource("xyz.openbmc_project.Control.Boot.Source.Sources.Network"), "Pxe");
}

TEST(dbusToRfBootSource, RemovableMedia)
{
  EXPECT_EQ(dbusToRfBootSource("xyz.openbmc_project.Control.Boot.Source.Sources.RemovableMedia"), "Usb");
}

TEST(dbusToRfBootSource, Unknown)
{
  EXPECT_EQ(dbusToRfBootSource("Unknown"), "");
}

TEST(dbusToRfBootType, Legacy)
{
  EXPECT_EQ(dbusToRfBootType("xyz.openbmc_project.Control.Boot.Type.Types.Legacy"), "Legacy");
}

TEST(dbusToRfBootType, EFI)
{
  EXPECT_EQ(dbusToRfBootType("xyz.openbmc_project.Control.Boot.Type.Types.EFI"), "UEFI");
}

TEST(dbusToRfBootType, Unknown)
{
  EXPECT_EQ(dbusToRfBootType("Unknown"), "");
}

TEST(dbusToRfBootMode, Regular)
{
  EXPECT_EQ(dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Regular"), "None");
}

TEST(dbusToRfBootMode, Safe)
{
  EXPECT_EQ(dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Safe"), "Diags");
}

TEST(dbusToRfBootMode, Setup)
{
  EXPECT_EQ(dbusToRfBootMode("xyz.openbmc_project.Control.Boot.Mode.Modes.Setup"), "BiosSetup");
}

TEST(dbusToRfBootMode, Unknown)
{
  EXPECT_EQ(dbusToRfBootMode("Unknown"), "");
}

} // namespace
} // namespace redfish
