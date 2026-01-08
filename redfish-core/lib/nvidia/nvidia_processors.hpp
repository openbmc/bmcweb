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

#include <memory>
#include <string>

namespace redfish
{

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

    // OEM framework will automatically place this under Oem/Nvidia
    nlohmann::json& oemNvidia = asyncResp->res.jsonValue;
    oemNvidia["@odata.id"] =
        boost::urls::format("/redfish/v1/Systems/{}/Processors/{}#/Oem/Nvidia",
                            systemId, processorId);

    // Query D-Bus for DefaultBoostClockSpeedMHz
    std::string objPath = "/xyz/openbmc_project/inventory/" + processorId;

    dbus::utility::getDbusObject(
        objPath,
        std::array<std::string_view, 1>{
            "xyz.openbmc_project.Inventory.Item.Accelerator"},
        [asyncResp, processorId,
         objPath](const boost::system::error_code& ec,
                  const dbus::utility::MapperGetObject& object) {
            if (ec || object.empty())
            {
                BMCWEB_LOG_DEBUG("No Accelerator object found for {}",
                                 processorId);
                return;
            }

            const std::string& service = object.begin()->first;

            dbus::utility::getAllProperties(
                service, objPath, "",
                [asyncResp](
                    const boost::system::error_code& ec2,
                    const dbus::utility::DBusPropertiesMap& properties) {
                    if (ec2)
                    {
                        BMCWEB_LOG_ERROR("D-Bus response error: {}", ec2);
                        return;
                    }

                    const uint64_t* boostClockFrequency = nullptr;
                    const bool success = sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), properties,
                        "BoostClockFrequency", boostClockFrequency);

                    if (!success)
                    {
                        return;
                    }

                    if (boostClockFrequency != nullptr &&
                        *boostClockFrequency > 0 &&
                        *boostClockFrequency !=
                            std::numeric_limits<uint64_t>::max())
                    {
                        // Framework automatically places this under Oem/Nvidia
                        asyncResp->res.jsonValue["BoostClockFrequency"] =
                            *boostClockFrequency;
                    }
                });
        });
}

inline void requestRoutesNvidiaProcessors(RedfishService& service)
{
    REDFISH_SUB_ROUTE<
        "/redfish/v1/Systems/<str>/Processors/<str>/#/Oem/Nvidia">(
        service, HttpVerb::Get)(handleGetProcessorNvidia);
}

} // namespace redfish
