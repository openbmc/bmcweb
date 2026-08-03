// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/asset_utils.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <cmath>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

constexpr std::array<std::string_view, 1> cableInterfaces = {
    "xyz.openbmc_project.Inventory.Item.Cable"};

constexpr auto cablePowerStateOn =
    "xyz.openbmc_project.State.Decorator.PowerState.State.On";
constexpr auto cablePowerStateOff =
    "xyz.openbmc_project.State.Decorator.PowerState.State.Off";
constexpr auto cablePowerStatePoweringOn =
    "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOn";
constexpr auto cablePowerStatePoweringOff =
    "xyz.openbmc_project.State.Decorator.PowerState.State.PoweringOff";
constexpr auto cablePowerStateUnknown =
    "xyz.openbmc_project.State.Decorator.PowerState.State.Unknown";

/**
 * @brief Fill cable specific properties.
 * @param[in,out]   asyncResp   Async HTTP response.
 * @param[in]       ec          Error code corresponding to Async method call.
 * @param[in]       properties  List of Cable Properties key/value pairs.
 */
inline void fillCableProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string* cableTypeDescription = nullptr;
    const double* length = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "CableTypeDescription",
        cableTypeDescription, "Length", length);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (cableTypeDescription != nullptr)
    {
        asyncResp->res.jsonValue["CableType"] = *cableTypeDescription;
    }

    if (length != nullptr)
    {
        if (!std::isfinite(*length))
        {
            // Cable length is NaN by default, do not throw an error
            if (!std::isnan(*length))
            {
                BMCWEB_LOG_ERROR("Cable length value is invalid");
                messages::internalError(asyncResp->res);
                return;
            }
        }
        else
        {
            asyncResp->res.jsonValue["LengthMeters"] = *length;
        }
    }
}

class CableStateAggregator
{
  public:
    explicit CableStateAggregator(
        std::shared_ptr<bmcweb::AsyncResp> asyncRespIn) :
        asyncResp(std::move(asyncRespIn))
    {}

    CableStateAggregator(const CableStateAggregator&) = delete;
    CableStateAggregator(CableStateAggregator&&) = delete;
    CableStateAggregator& operator=(const CableStateAggregator&) = delete;
    CableStateAggregator& operator=(CableStateAggregator&&) = delete;

    ~CableStateAggregator()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }
        asyncResp->res.jsonValue["Status"]["State"] =
            present ? poweredState : resource::State::Absent;
    }

    void setPresent(bool value)
    {
        present = value;
    }

    void setPoweredState(resource::State value)
    {
        poweredState = value;
    }

  private:
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    bool present = true;
    resource::State poweredState = resource::State::Enabled;
};

/**
 * @brief Record whether the cable is present.
 *
 * phosphor-dbus-interfaces documents a "Present" of false as mapping to a
 * Redfish "Status" "State" of "Absent".
 */
inline void fillCablePresence(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<CableStateAggregator>& cableState,
    const std::string& cableObjectPath, const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, cableObjectPath, "xyz.openbmc_project.Inventory.Item",
        "Present",
        // ast-grep-ignore: long-lambda
        [asyncResp, cableState,
         cableObjectPath](const boost::system::error_code& ec, bool present) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "get presence failed for Cable {} with error {}",
                        cableObjectPath, ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            cableState->setPresent(present);
        });
}

/**
 * @brief Derive Status.State from the power state decorator.
 *
 * A cable whose attached card is powered off is neither faulty nor missing,
 * so it is reported as StandbyOffline rather than Disabled or Absent. 
 * An unknown or unimplemented power state leaves State at its default rather
 * than asserting a fault from absence of information.
 */
inline void fillCablePowerState(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::shared_ptr<CableStateAggregator>& cableState,
    const std::string& cableObjectPath, const std::string& service)
{
    dbus::utility::getProperty<std::string>(
        service, cableObjectPath,
        "xyz.openbmc_project.State.Decorator.PowerState", "PowerState",
        // ast-grep-ignore: long-lambda
        [asyncResp, cableState,
         cableObjectPath](const boost::system::error_code& ec,
                          const std::string& powerState) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "get PowerState failed for Cable {} with error {}",
                        cableObjectPath, ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (powerState == cablePowerStateOn)
            {
                cableState->setPoweredState(resource::State::Enabled);
            }
            else if (powerState == cablePowerStatePoweringOn)
            {
                cableState->setPoweredState(resource::State::Starting);
            }
            else if (powerState == cablePowerStateOff ||
                     powerState == cablePowerStatePoweringOff)
            {
                cableState->setPoweredState(resource::State::StandbyOffline);
            }
            else if (powerState != cablePowerStateUnknown)
            {
                BMCWEB_LOG_ERROR("Unknown PowerState {} for Cable {}",
                                 powerState, cableObjectPath);
                messages::internalError(asyncResp->res);
            }
        });
}

/**
 * @brief Fill Status.Health from the operational status decorator.
 *
 * phosphor-dbus-interfaces documents a "Functional" of false as mapping to a
 * Redfish "Status" "Health" of "Critical".  The decorator is optional, so
 * Health is left at its default when it is not implemented.
 *
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cableObjectPath Object path of the Cable.
 * @param[in]       service         Service hosting the decorator.
 */
inline void fillCableHealth(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& cableObjectPath,
                            const std::string& service)
{
    dbus::utility::getProperty<bool>(
        service, cableObjectPath,
        "xyz.openbmc_project.State.Decorator.OperationalStatus", "Functional",
        // ast-grep-ignore: long-lambda
        [asyncResp, cableObjectPath](const boost::system::error_code& ec,
                                     bool functional) {
            if (ec)
            {
                if (ec.value() != EBADR)
                {
                    BMCWEB_LOG_ERROR(
                        "get Functional failed for Cable {} with error {}",
                        cableObjectPath, ec.value());
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            if (!functional)
            {
                asyncResp->res.jsonValue["Status"]["Health"] =
                    resource::Health::Critical;
            }
        });
}

/**
 * @brief Api to get Cable properties.
 * @param[in,out]   asyncResp       Async HTTP response.
 * @param[in]       cableObjectPath Object path of the Cable.
 * @param[in]       serviceMap      A map to hold Service and corresponding
 * interface list for the given cable id.
 */
inline void getCableProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cableObjectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    BMCWEB_LOG_DEBUG("Get Properties for cable {}", cableObjectPath);

    auto cableState = std::make_shared<CableStateAggregator>(asyncResp);

    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface == "xyz.openbmc_project.Inventory.Item.Cable")
            {
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, service, cableObjectPath,
                    interface, std::bind_front(fillCableProperties, asyncResp));
            }
            else if (interface ==
                     "xyz.openbmc_project.Inventory.Decorator.Asset")
            {
                asset_utils::getAssetInfo(asyncResp, service, cableObjectPath,
                                          ""_json_pointer, false, true);
            }
            else if (interface == "xyz.openbmc_project.Inventory.Item")
            {
                fillCablePresence(asyncResp, cableState, cableObjectPath,
                                  service);
            }
            else if (interface ==
                     "xyz.openbmc_project.State.Decorator.PowerState")
            {
                fillCablePowerState(asyncResp, cableState, cableObjectPath,
                                    service);
            }
            else if (interface ==
                     "xyz.openbmc_project.State.Decorator.OperationalStatus")
            {
                fillCableHealth(asyncResp, cableObjectPath, service);
            }
        }
    }
}

inline void afterHandleCableGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& cableId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "Cable", cableId);
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::object_path path(objectPath);
        if (path.filename() != cableId)
        {
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] = "#Cable.v1_0_0.Cable";
        asyncResp->res.jsonValue["@odata.id"] =
            boost::urls::format("/redfish/v1/Cables/{}", cableId);
        asyncResp->res.jsonValue["Id"] = cableId;
        asyncResp->res.jsonValue["Name"] = "Cable";
        asyncResp->res.jsonValue["Status"]["Health"] = resource::Health::OK;

        getCableProperties(asyncResp, objectPath, serviceMap);
        return;
    }
    messages::resourceNotFound(asyncResp->res, "Cable", cableId);
}

inline void handleCableGet(App& app, const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& cableId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG("Cable Id: {}", cableId);

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/inventory", 0, cableInterfaces,
        std::bind_front(afterHandleCableGet, asyncResp, cableId));
}

inline void handleCableCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#CableCollection.CableCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Cables";
    asyncResp->res.jsonValue["Name"] = "Cable Collection";
    asyncResp->res.jsonValue["Description"] = "Collection of Cable Entries";
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/Cables"), cableInterfaces,
        "/xyz/openbmc_project/inventory");
}

/**
 * The Cable schema
 */
inline void requestRoutesCable(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/<str>/")
        .privileges(redfish::privileges::getCable)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleCableGet, std::ref(app)));
}

/**
 * Collection of Cable resource instances
 */
inline void requestRoutesCableCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Cables/")
        .privileges(redfish::privileges::getCableCollection)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleCableCollectionGet, std::ref(app)));
}

} // namespace redfish
