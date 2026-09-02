// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/leak_detector.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace redfish
{

constexpr std::array<std::string_view, 1> leakDetectorInterface = {
    "xyz.openbmc_project.State.Leak.Detector"};

inline void getLeakDetectorSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& validChassisPath,
    std::function<void(const dbus::utility::MapperGetSubTreeResponse&)>&&
        callback)
{
    sdbusplus::object_path endpointPath{validChassisPath};
    endpointPath /= "monitored_by";

    dbus::utility::getAssociatedSubTree(
        endpointPath,
        sdbusplus::object_path("/xyz/openbmc_project/state/leak/detector"), 0,
        leakDetectorInterface,
        [asyncResp, callback = std::move(callback)](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
            if (ec)
            {
                if (ec.value() != boost::system::errc::io_error &&
                    ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR("DBUS response error {}", ec);
                    messages::internalError(asyncResp->res);
                    return;
                }
                callback({});
                return;
            }
            callback(subtree);
        });
}

inline void handleLeakDetectorCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp,
         chassisId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#LeakDetectorCollection.LeakDetectorCollection";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors",
                chassisId);
            asyncResp->res.jsonValue["Name"] = "Leak Detector Collection";
            asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
            asyncResp->res.jsonValue["Members@odata.count"] = 0;

            getLeakDetectorSubTree(
                asyncResp, *validChassisPath,
                [asyncResp, chassisId](
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    nlohmann::json& members =
                        asyncResp->res.jsonValue["Members"];
                    for (const auto& [path, services] : subtree)
                    {
                        std::string name =
                            sdbusplus::object_path(path).filename();
                        if (name.empty())
                        {
                            continue;
                        }

                        nlohmann::json item = nlohmann::json::object();
                        item["@odata.id"] = boost::urls::format(
                            "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors/{}",
                            chassisId, name);
                        members.emplace_back(std::move(item));
                    }
                    asyncResp->res.jsonValue["Members@odata.count"] =
                        members.size();
                });
        });
}

inline leak_detector::DetectorState toDetectorState(std::string_view state)
{
    if (state == "xyz.openbmc_project.State.Leak.Detector.DetectorState.Normal")
    {
        return leak_detector::DetectorState::OK;
    }
    if (state ==
        "xyz.openbmc_project.State.Leak.Detector.DetectorState.Abnormal")
    {
        return leak_detector::DetectorState::Critical;
    }
    return leak_detector::DetectorState::Unavailable;
}

inline leak_detector::LeakDetectorType toDetectorType(std::string_view type)
{
    if (type ==
        "xyz.openbmc_project.State.Leak.Detector.DetectorType.LeakSensingCable")
    {
        return leak_detector::LeakDetectorType::Moisture;
    }
    return leak_detector::LeakDetectorType::Invalid;
}

inline void afterGetLeakDetector(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& detectorId,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
        }
        return;
    }

    const std::string* prettyName = nullptr;
    const std::string* state = nullptr;
    const std::string* type = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "PrettyName",
            prettyName, "State", state, "Type", type))
    {
        messages::internalError(asyncResp->res);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/LeakDetector/LeakDetector.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#LeakDetector.v1_6_0.LeakDetector";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors/{}",
        chassisId, detectorId);
    asyncResp->res.jsonValue["Id"] = detectorId;
    asyncResp->res.jsonValue["Name"] =
        prettyName != nullptr ? *prettyName : detectorId;

    if (state != nullptr)
    {
        asyncResp->res.jsonValue["DetectorState"] = toDetectorState(*state);
    }

    if (type != nullptr)
    {
        leak_detector::LeakDetectorType detectorType = toDetectorType(*type);
        if (detectorType != leak_detector::LeakDetectorType::Invalid)
        {
            asyncResp->res.jsonValue["LeakDetectorType"] = detectorType;
        }
    }

    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;
}

inline void handleLeakDetectorGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& detectorId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    redfish::chassis_utils::getValidChassisPath(
        asyncResp, chassisId,
        [asyncResp, chassisId,
         detectorId](const std::optional<std::string>& validChassisPath) {
            if (!validChassisPath)
            {
                messages::resourceNotFound(asyncResp->res, "Chassis",
                                           chassisId);
                return;
            }

            getLeakDetectorSubTree(
                asyncResp, *validChassisPath,
                [asyncResp, chassisId, detectorId](
                    const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    auto found = std::ranges::find_if(
                        subtree, [&detectorId](const auto& entry) {
                            return sdbusplus::object_path(entry.first)
                                       .filename() == detectorId;
                        });

                    if (found == subtree.end() || found->second.empty())
                    {
                        messages::resourceNotFound(asyncResp->res,
                                                   "LeakDetector", detectorId);
                        return;
                    }

                    dbus::utility::getAllProperties(
                        found->second.front().first, found->first,
                        "xyz.openbmc_project.State.Leak.Detector",
                        std::bind_front(afterGetLeakDetector, asyncResp,
                                        chassisId, detectorId));
                });
        });
}

inline void requestRoutesLeakDetectorCollection(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/")
        .privileges(redfish::privileges::getLeakDetectorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLeakDetectorCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/<str>/")
        .privileges(redfish::privileges::getLeakDetector)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLeakDetectorGet, std::ref(app)));
}

} // namespace redfish
