#pragma once

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "redfish.hpp"
#include "sub_request.hpp"
#include "utils/dbus_utils.hpp"
#include "verb.hpp"

#include <functional>
#include <limits>
#include <memory>
#include <string>

namespace redfish
{

inline void handleBoostClockFrequencyResponse(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("D-Bus response error: {}", ec);
        return;
    }

    const uint64_t* boostClockFrequency = nullptr;
    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "BoostClockFrequency",
        boostClockFrequency);

    if (!success)
    {
        return;
    }

    if (boostClockFrequency != nullptr && *boostClockFrequency > 0 &&
        *boostClockFrequency != std::numeric_limits<uint64_t>::max())
    {
        asyncResp->res.jsonValue["BoostClockFrequency"] = *boostClockFrequency;
    }
}

inline void handleAcceleratorObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& processorId, const std::string& objPath,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec || object.empty())
    {
        BMCWEB_LOG_DEBUG("No Accelerator object found for {}", processorId);
        return;
    }

    const std::string& service = object.begin()->first;

    dbus::utility::getAllProperties(
        service, objPath, "",
        std::bind_front(handleBoostClockFrequencyResponse, asyncResp));
}

inline void handleGetProcessorNvidia(
    const redfish::SubRequest& /*req*/,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& systemId, const std::string& processorId)
{
    if (systemId != "system")
    {
        messages::resourceNotFound(asyncResp->res, "ComputerSystem", systemId);
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] = "#NvidiaProcessor.v1_0_0.Oem";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}#/Oem/Nvidia",
                            systemId, processorId);

    std::string objPath = "/xyz/openbmc_project/inventory/" + processorId;

    dbus::utility::getDbusObject(
        objPath,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Item.Accelerator"},
        std::bind_front(handleAcceleratorObject, asyncResp, processorId,
                        objPath));
}

inline void requestRoutesNvidiaProcessors(RedfishService& service)
{
    REDFISH_SUB_ROUTE<
        "/redfish/v1/Systems/<str>/Processors/<str>/#/Oem/Nvidia">(
        service, HttpVerb::Get)(handleGetProcessorNvidia);
}

} // namespace redfish
