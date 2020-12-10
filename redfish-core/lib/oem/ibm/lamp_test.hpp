#pragma once

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <variant>

namespace redfish
{

/**
 * @brief Retrieves lamp test state.
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
inline void getLampTestState(const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{

    BMCWEB_LOG_DEBUG << "Get lamp test state";

    crow::connections::systemBus->async_method_call(
        [aResp](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
                messages::internalError(aResp->res);
                return;
            }

            if (getObjectType.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Can't find led D-Bus object!";
                messages::internalError(aResp->res);
                return;
            }

            if (getObjectType[0].first.empty())
            {
                BMCWEB_LOG_DEBUG << "Error getting led D-Bus object!";
                messages::internalError(aResp->res);
                return;
            }

            const std::string service = getObjectType[0].first;
            BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec,
                        std::variant<bool>& asserted) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG << "DBUS response error " << ec;
                        messages::internalError(aResp->res);
                        return;
                    }

                    const bool* assertedPtr = std::get_if<bool>(&asserted);

                    if (!assertedPtr)
                    {
                        BMCWEB_LOG_DEBUG << "Can't get lamp test status!";
                        messages::internalError(aResp->res);
                        return;
                    }
                    aResp->res.jsonValue["Oem"]["@odata.type"] =
                        "#OemComputerSystem.Oem";
                    aResp->res.jsonValue["Oem"]["IBM"]["@odata.type"] =
                        "#OemComputerSystem.IBM";
                    aResp->res.jsonValue["Oem"]["IBM"]["LampTest"] =
                        *assertedPtr;
                },
                service, "/xyz/openbmc_project/led/groups/lamp_test",
                "org.freedesktop.DBus.Properties", "Get",
                "xyz.openbmc_project.Led.Group", "Asserted");
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/led/groups/lamp_test",
        std::array<const char*, 1>{"xyz.openbmc_project.Led.Group"});
}

/**
 * @brief Sets lamp test state.
 *
 * @param[in] aResp   Shared pointer for generating response message.
 * @param[in] state   Lamp test state from request.
 *
 * @return None.
 */
inline void setLampTestState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                             const bool state)
{
    BMCWEB_LOG_DEBUG << "Set lamp test status.";

    crow::connections::systemBus->async_method_call(
        [aResp, state](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>&
                getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                 << ec;
                messages::internalError(aResp->res);
                return;
            }

            if (getObjectType.size() != 1)
            {
                BMCWEB_LOG_DEBUG << "Can't find led D-Bus object!";
                messages::internalError(aResp->res);
                return;
            }

            if (getObjectType[0].first.empty())
            {
                BMCWEB_LOG_DEBUG << "Error getting led D-Bus object!!";
                messages::internalError(aResp->res);
                return;
            }

            const std::string service = getObjectType[0].first;
            BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

            crow::connections::systemBus->async_method_call(
                [aResp](const boost::system::error_code ec) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG
                            << "Can't set lamp test status. Error: " << ec;
                        messages::internalError(aResp->res);
                        return;
                    }
                },
                service, "/xyz/openbmc_project/led/groups/lamp_test",
                "org.freedesktop.DBus.Properties", "Set",
                "xyz.openbmc_project.Led.Group", "Asserted",
                std::variant<bool>(state));
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        "/xyz/openbmc_project/led/groups/lamp_test",
        std::array<const char*, 1>{"xyz.openbmc_project.Led.Group"});
}

} // namespace redfish
