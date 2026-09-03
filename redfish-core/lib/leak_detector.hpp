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
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
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
#include <utility>

namespace redfish
{

constexpr std::array<std::string_view, 1> leakDetectorInterface = {
    "xyz.openbmc_project.State.Leak.Detector"};

inline void afterGetLeakDetectorSubTree(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::function<void(const dbus::utility::MapperGetSubTreeResponse&)>&
        callback,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec)
    {
        if (ec.value() != boost::system::errc::io_error && ec.value() != EBADR)
        {
            BMCWEB_LOG_ERROR("DBUS response error {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }
        BMCWEB_LOG_DEBUG("No leak detector association found");
        callback({});
        return;
    }

    callback(subtree);
}

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
        std::bind_front(afterGetLeakDetectorSubTree, asyncResp,
                        std::move(callback)));
}

inline void afterGetLeakDetectorCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::urls::url& collectionPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreePathsResponse& paths)
{
    if (ec && ec.value() != boost::system::errc::io_error &&
        ec.value() != EBADR)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }
    if (ec)
    {
        BMCWEB_LOG_DEBUG("No leak detector association found");
    }

    collection_util::handleCollectionMembers(
        asyncResp, collectionPath, nlohmann::json::json_pointer("/Members"),
        boost::system::error_code{}, paths);
}

inline void afterGetValidChassisForCollection(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/LeakDetectorCollection/LeakDetectorCollection.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#LeakDetectorCollection.LeakDetectorCollection";
    asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors",
        chassisId);
    asyncResp->res.jsonValue["Name"] = "Leak Detector Collection";

    boost::urls::url collectionPath = boost::urls::format(
        "/redfish/v1/Chassis/{}/ThermalSubsystem/LeakDetection/LeakDetectors",
        chassisId);

    dbus::utility::getAssociatedSubTreePaths(
        sdbusplus::object_path(*validChassisPath) / "monitored_by",
        sdbusplus::object_path("/xyz/openbmc_project/state/leak/detector"), 0,
        leakDetectorInterface,
        std::bind_front(afterGetLeakDetectorCollection, asyncResp,
                        std::move(collectionPath)));
}

inline void afterGetValidChassisForCollectionHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/LeakDetectorCollection/LeakDetectorCollection.json>; rel=describedby");
}

inline void handleLeakDetectorCollectionHead(
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
        std::bind_front(afterGetValidChassisForCollectionHead, asyncResp,
                        chassisId));
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
        std::bind_front(afterGetValidChassisForCollection, asyncResp,
                        chassisId));
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

inline resource::Health toDetectorHealth(leak_detector::DetectorState state)
{
    if (state == leak_detector::DetectorState::Critical)
    {
        return resource::Health::Critical;
    }
    return resource::Health::OK;
}

inline leak_detector::LeakDetectorType toDetectorType(std::string_view type)
{
    if (type ==
        "xyz.openbmc_project.State.Leak.Detector.DetectorType.LeakSensingCable")
    {
        return leak_detector::LeakDetectorType::Moisture;
    }
    if (type == "xyz.openbmc_project.State.Leak.Detector.DetectorType.Unknown")
    {
        return leak_detector::LeakDetectorType::Invalid;
    }
    BMCWEB_LOG_ERROR("Unrecognised leak detector type {}", type);
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
        if (ec.value() == EBADR)
        {
            BMCWEB_LOG_DEBUG("Leak detector {} is not on D-Bus", detectorId);
            messages::resourceNotFound(asyncResp->res, "LeakDetector",
                                       detectorId);
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
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

    asyncResp->res.jsonValue["Status"]["State"] = resource::State::Enabled;

    if (state != nullptr)
    {
        const leak_detector::DetectorState detectorState =
            toDetectorState(*state);
        asyncResp->res.jsonValue["DetectorState"] = detectorState;

        if (detectorState == leak_detector::DetectorState::Unavailable)
        {
            asyncResp->res.jsonValue["Status"]["State"] =
                resource::State::UnavailableOffline;
        }
        else
        {
            asyncResp->res.jsonValue["Status"]["Health"] =
                toDetectorHealth(detectorState);
        }
    }

    if (type != nullptr)
    {
        leak_detector::LeakDetectorType detectorType = toDetectorType(*type);
        if (detectorType != leak_detector::LeakDetectorType::Invalid)
        {
            asyncResp->res.jsonValue["LeakDetectorType"] = detectorType;
        }
    }
}

inline dbus::utility::MapperGetSubTreeResponse::const_iterator findLeakDetector(
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::string& detectorId)
{
    return std::ranges::find_if(subtree, [&detectorId](const auto& entry) {
        return sdbusplus::object_path(entry.first).filename() == detectorId;
    });
}

inline void afterGetLeakDetectorSubTreeForDetector(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& detectorId,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    auto found = findLeakDetector(subtree, detectorId);

    if (found == subtree.end() || found->second.empty())
    {
        messages::resourceNotFound(asyncResp->res, "LeakDetector", detectorId);
        return;
    }

    dbus::utility::getAllProperties(
        found->second.front().first, found->first,
        "xyz.openbmc_project.State.Leak.Detector",
        std::bind_front(afterGetLeakDetector, asyncResp, chassisId,
                        detectorId));
}

inline void afterGetValidChassisForDetector(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& detectorId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getLeakDetectorSubTree(
        asyncResp, *validChassisPath,
        std::bind_front(afterGetLeakDetectorSubTreeForDetector, asyncResp,
                        chassisId, detectorId));
}

inline void afterGetLeakDetectorSubTreeForDetectorHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& detectorId,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    auto found = findLeakDetector(subtree, detectorId);

    if (found == subtree.end() || found->second.empty())
    {
        messages::resourceNotFound(asyncResp->res, "LeakDetector", detectorId);
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/LeakDetector/LeakDetector.json>; rel=describedby");
}

inline void afterGetValidChassisForDetectorHead(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& chassisId, const std::string& detectorId,
    const std::optional<std::string>& validChassisPath)
{
    if (!validChassisPath)
    {
        messages::resourceNotFound(asyncResp->res, "Chassis", chassisId);
        return;
    }

    getLeakDetectorSubTree(
        asyncResp, *validChassisPath,
        std::bind_front(afterGetLeakDetectorSubTreeForDetectorHead, asyncResp,
                        detectorId));
}

inline void handleLeakDetectorHead(
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
        std::bind_front(afterGetValidChassisForDetectorHead, asyncResp,
                        chassisId, detectorId));
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
        std::bind_front(afterGetValidChassisForDetector, asyncResp, chassisId,
                        detectorId));
}

inline void requestRoutesLeakDetectorCollection(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/")
        .privileges(redfish::privileges::headLeakDetectorCollection)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleLeakDetectorCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/")
        .privileges(redfish::privileges::getLeakDetectorCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLeakDetectorCollectionGet, std::ref(app)));
}

inline void requestRoutesLeakDetector(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/<str>/")
        .privileges(redfish::privileges::headLeakDetector)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleLeakDetectorHead, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Chassis/<str>/ThermalSubsystem/LeakDetection/LeakDetectors/<str>/")
        .privileges(redfish::privileges::getLeakDetector)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleLeakDetectorGet, std::ref(app)));
}

} // namespace redfish
