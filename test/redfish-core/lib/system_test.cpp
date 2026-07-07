// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"
#include "systems.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
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

TEST(AfterGetUUID, ErrorSetsInternalError)
{
    using nlohmann::literals::operator""_json;

    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetUUID(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue,
              R"({
        "error": {
            "@Message.ExtendedInfo": [
                {
                    "@odata.type": "#Message.v1_1_1.Message",
                    "Message": "The request failed due to an internal service error.  The service is still operational.",
                    "MessageArgs": [],
                    "MessageId": "Base.1.19.InternalError",
                    "MessageSeverity": "Critical",
                    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                }
            ],
            "code": "Base.1.19.InternalError",
            "message": "The request failed due to an internal service error.  The service is still operational."
        }
    })"_json);
}

TEST(AfterGetUUID, RawUUIDFormattedWithDashes)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"UUID", dbus::utility::DbusVariantType{
                     std::string{"a9e26b3ca5c94b9d8fe92e4c9d93fa9e"}}}};

    afterGetUUID(response, {}, properties);

    EXPECT_EQ(response->res.jsonValue.size(), 1);
    EXPECT_EQ(response->res.jsonValue["UUID"],
              "a9e26b3c-a5c9-4b9d-8fe9-2e4c9d93fa9e");
}

TEST(AfterGetUUID, PreFormattedUUIDIsUnchanged)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"UUID", dbus::utility::DbusVariantType{
                     std::string{"a9e26b3c-a5c9-4b9d-8fe9-2e4c9d93fa9e"}}}};

    afterGetUUID(response, {}, properties);

    EXPECT_EQ(response->res.jsonValue.size(), 1);
    EXPECT_EQ(response->res.jsonValue["UUID"],
              "a9e26b3c-a5c9-4b9d-8fe9-2e4c9d93fa9e");
}

TEST(AfterGetUUID, MissingUUIDPropertySetsNoField)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetUUID(response, {}, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue.size(), 0);
}

TEST(AfterGetUUID, InvalidPropertyTypeSetsInternalError)
{
    using nlohmann::literals::operator""_json;

    auto response = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"UUID", dbus::utility::DbusVariantType{static_cast<uint64_t>(42)}}};

    afterGetUUID(response, {}, properties);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue.size(), 1);
    EXPECT_EQ(response->res.jsonValue,
              R"({
        "error": {
            "@Message.ExtendedInfo": [
                {
                    "@odata.type": "#Message.v1_1_1.Message",
                    "Message": "The request failed due to an internal service error.  The service is still operational.",
                    "MessageArgs": [],
                    "MessageId": "Base.1.19.InternalError",
                    "MessageSeverity": "Critical",
                    "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                }
            ],
            "code": "Base.1.19.InternalError",
            "message": "The request failed due to an internal service error.  The service is still operational."
        }
    })"_json);
}

} // namespace
} // namespace redfish
