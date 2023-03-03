/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "openbmc_dbus_rest.hpp"
#include "query.hpp"
#include "redfish_util.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"
#include "utils/stl_utils.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>

#include <array>
#include <optional>
#include <string_view>
#include <variant>

namespace redfish
{

void getNTPProtocolEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
std::string getHostName();

constexpr std::string_view sshServiceName = "dropbear";
constexpr std::string_view httpsServiceName = "bmcweb";
constexpr std::string_view ipmiServiceName = "phosphor-ipmi-net";

// Mapping from Redfish NetworkProtocol key name to backend service that hosts
// that protocol.
constexpr std::array<std::pair<std::string_view, std::string_view>, 3>
    networkProtocolToDbus = {{{"SSH", sshServiceName},
                              {"HTTPS", httpsServiceName},
                              {"IPMI", ipmiServiceName}}};

inline void extractNTPServersAndDomainNamesData(
    const dbus::utility::ManagedObjectType& dbusData,
    std::vector<std::string>& ntpData, std::vector<std::string>& dnData)
{
    for (const auto& obj : dbusData)
    {
        for (const auto& ifacePair : obj.second)
        {
            if (ifacePair.first !=
                "xyz.openbmc_project.Network.EthernetInterface")
            {
                continue;
            }

            for (const auto& propertyPair : ifacePair.second)
            {
                if (propertyPair.first == "StaticNTPServers")
                {
                    const std::vector<std::string>* ntpServers =
                        std::get_if<std::vector<std::string>>(
                            &propertyPair.second);
                    if (ntpServers != nullptr)
                    {
                        ntpData = *ntpServers;
                    }
                }
                else if (propertyPair.first == "DomainName")
                {
                    const std::vector<std::string>* domainNames =
                        std::get_if<std::vector<std::string>>(
                            &propertyPair.second);
                    if (domainNames != nullptr)
                    {
                        dnData = *domainNames;
                    }
                }
            }
        }
    }
    stl_utils::removeDuplicate(ntpData);
}

template <typename CallbackFunc>
void getEthernetIfaceData(CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::forward<CallbackFunc>(callback)}](
            const boost::system::error_code& errorCode,
            const dbus::utility::ManagedObjectType& dbusData) {
        std::vector<std::string> ntpServers;
        std::vector<std::string> domainNames;

        if (errorCode)
        {
            callback(false, ntpServers, domainNames);
            return;
        }

        extractNTPServersAndDomainNamesData(dbusData, ntpServers, domainNames);

        callback(true, ntpServers, domainNames);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void afterNetworkPortRequest(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const std::vector<std::tuple<std::string, std::string, bool>>& socketData)
{
    if (ec)
    {
        messages::internalError(asyncResp->res);
        return;
    }
    for (const auto& data : socketData)
    {
        const std::string& socketPath = get<0>(data);
        const std::string& protocolName = get<1>(data);
        bool isProtocolEnabled = get<2>(data);

        asyncResp->res.jsonValue[protocolName]["ProtocolEnabled"] =
            isProtocolEnabled;
        asyncResp->res.jsonValue[protocolName]["Port"] = nullptr;
        getPortNumber(socketPath, [asyncResp, protocolName](
                                      const boost::system::error_code& ec2,
                                      int portNumber) {
            if (ec2)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            asyncResp->res.jsonValue[protocolName]["Port"] = portNumber;
        });
    }
}

inline void getNetworkData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const crow::Request& req)
{
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerNetworkProtocol/NetworkProtocol.json>; rel=describedby");
    asyncResp->res.jsonValue["@odata.type"] =
        "#ManagerNetworkProtocol.v1_5_0.ManagerNetworkProtocol";
    asyncResp->res.jsonValue["@odata.id"] =
        "/redfish/v1/Managers/bmc/NetworkProtocol";
    asyncResp->res.jsonValue["Id"] = "NetworkProtocol";
    asyncResp->res.jsonValue["Name"] = "Manager Network Protocol";
    asyncResp->res.jsonValue["Description"] = "Manager Network Service";
    asyncResp->res.jsonValue["Status"]["Health"] = "OK";
    asyncResp->res.jsonValue["Status"]["HealthRollup"] = "OK";
    asyncResp->res.jsonValue["Status"]["State"] = "Enabled";

    // HTTP is Mandatory attribute as per OCP Baseline Profile - v1.0.0,
    // but from security perspective it is not recommended to use.
    // Hence using protocolEnabled as false to make it OCP and security-wise
    // compliant
    asyncResp->res.jsonValue["HTTP"]["Port"] = nullptr;
    asyncResp->res.jsonValue["HTTP"]["ProtocolEnabled"] = false;

    std::string hostName = getHostName();

    asyncResp->res.jsonValue["HostName"] = hostName;

    getNTPProtocolEnabled(asyncResp);

    getEthernetIfaceData(
        [hostName, asyncResp](const bool& success,
                              const std::vector<std::string>& ntpServers,
                              const std::vector<std::string>& domainNames) {
        if (!success)
        {
            messages::resourceNotFound(asyncResp->res, "ManagerNetworkProtocol",
                                       "NetworkProtocol");
            return;
        }
        asyncResp->res.jsonValue["NTP"]["NTPServers"] = ntpServers;
        if (!hostName.empty())
        {
            std::string fqdn = hostName;
            if (!domainNames.empty())
            {
                fqdn += ".";
                fqdn += domainNames[0];
            }
            asyncResp->res.jsonValue["FQDN"] = std::move(fqdn);
        }
    });

    Privileges effectiveUserPrivileges =
        redfish::getUserPrivileges(req.userRole);

    // /redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates is
    // something only ConfigureManager can access then only display when
    // the user has permissions ConfigureManager
    if (isOperationAllowedWithPrivileges({{"ConfigureManager"}},
                                         effectiveUserPrivileges))
    {
        asyncResp->res.jsonValue["HTTPS"]["Certificates"]["@odata.id"] =
            "/redfish/v1/Managers/bmc/NetworkProtocol/HTTPS/Certificates";
    }

    getPortStatusAndPath(std::span(networkProtocolToDbus),
                         std::bind_front(afterNetworkPortRequest, asyncResp));
} // namespace redfish

inline void handleNTPProtocolEnabled(
    const bool& ntpEnabled, const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string timeSyncMethod;
    if (ntpEnabled)
    {
        timeSyncMethod = "xyz.openbmc_project.Time.Synchronization.Method.NTP";
    }
    else
    {
        timeSyncMethod =
            "xyz.openbmc_project.Time.Synchronization.Method.Manual";
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& errorCode) {
        if (errorCode)
        {
            messages::internalError(asyncResp->res);
        }
        },
        "xyz.openbmc_project.Settings", "/xyz/openbmc_project/time/sync_method",
        "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Time.Synchronization", "TimeSyncMethod",
        dbus::utility::DbusVariantType{timeSyncMethod});
}

inline void
    handleNTPServersPatch(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::vector<nlohmann::json>& ntpServerObjects,
                          std::vector<std::string> currentNtpServers)
{
    std::vector<std::string>::iterator currentNtpServer =
        currentNtpServers.begin();
    for (size_t index = 0; index < ntpServerObjects.size(); index++)
    {
        const nlohmann::json& ntpServer = ntpServerObjects[index];
        if (ntpServer.is_null())
        {
            // Can't delete an item that doesn't exist
            if (currentNtpServer == currentNtpServers.end())
            {
                messages::propertyValueNotInList(asyncResp->res, "null",
                                                 "NTP/NTPServers/" +
                                                     std::to_string(index));

                return;
            }
            currentNtpServer = currentNtpServers.erase(currentNtpServer);
            continue;
        }
        const nlohmann::json::object_t* ntpServerObject =
            ntpServer.get_ptr<const nlohmann::json::object_t*>();
        if (ntpServerObject != nullptr)
        {
            if (!ntpServerObject->empty())
            {
                messages::propertyValueNotInList(
                    asyncResp->res,
                    ntpServer.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                    "NTP/NTPServers/" + std::to_string(index));
                return;
            }
            // Can't retain an item that doesn't exist
            if (currentNtpServer == currentNtpServers.end())
            {
                messages::propertyValueOutOfRange(
                    asyncResp->res,
                    ntpServer.dump(2, ' ', true,
                                   nlohmann::json::error_handler_t::replace),
                    "NTP/NTPServers/" + std::to_string(index));

                return;
            }
            // empty objects should leave the NtpServer unmodified
            currentNtpServer++;
            continue;
        }

        const std::string* ntpServerStr =
            ntpServer.get_ptr<const std::string*>();
        if (ntpServerStr == nullptr)
        {
            messages::propertyValueTypeError(
                asyncResp->res,
                ntpServer.dump(2, ' ', true,
                               nlohmann::json::error_handler_t::replace),
                "NTP/NTPServers/" + std::to_string(index));
            return;
        }
        if (currentNtpServer == currentNtpServers.end())
        {
            // if we're at the end of the list, append to the end
            currentNtpServers.push_back(*ntpServerStr);
            currentNtpServer = currentNtpServers.end();
            continue;
        }
        *currentNtpServer = *ntpServerStr;
        currentNtpServer++;
    }

    // Any remaining array elements should be removed
    currentNtpServers.erase(currentNtpServer, currentNtpServers.end());

    constexpr std::array<std::string_view, 1> ethInterfaces = {
        "xyz.openbmc_project.Network.EthernetInterface"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project", 0, ethInterfaces,
        [asyncResp, currentNtpServers](
            const boost::system::error_code& ec,
            const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            BMCWEB_LOG_WARNING << "D-Bus error: " << ec << ", " << ec.message();
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& [objectPath, serviceMap] : subtree)
        {
            for (const auto& [service, interfaces] : serviceMap)
            {
                for (const auto& interface : interfaces)
                {
                    if (interface !=
                        "xyz.openbmc_project.Network.EthernetInterface")
                    {
                        continue;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp](const boost::system::error_code& ec2) {
                        if (ec2)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        },
                        service, objectPath, "org.freedesktop.DBus.Properties",
                        "Set", interface, "StaticNTPServers",
                        dbus::utility::DbusVariantType{currentNtpServers});
                }
            }
        }
        });
}

inline void
    handleProtocolEnabled(const bool protocolEnabled,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& netBasePath)
{
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Control.Service.Attributes"};
    dbus::utility::getSubTree(
        "/xyz/openbmc_project/control/service", 0, interfaces,
        [protocolEnabled, asyncResp,
         netBasePath](const boost::system::error_code& ec,
                      const dbus::utility::MapperGetSubTreeResponse& subtree) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& entry : subtree)
        {
            if (boost::algorithm::starts_with(entry.first, netBasePath))
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code& ec2) {
                    if (ec2)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    },
                    entry.second.begin()->first, entry.first,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Service.Attributes", "Running",
                    dbus::utility::DbusVariantType{protocolEnabled});

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code& ec2) {
                    if (ec2)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    },
                    entry.second.begin()->first, entry.first,
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.Control.Service.Attributes", "Enabled",
                    dbus::utility::DbusVariantType{protocolEnabled});
            }
        }
        });
}

inline std::string getHostName()
{
    std::string hostName;

    std::array<char, HOST_NAME_MAX> hostNameCStr{};
    if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0)
    {
        hostName = hostNameCStr.data();
    }
    return hostName;
}

inline void
    getNTPProtocolEnabled(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    sdbusplus::asio::getProperty<std::string>(
        *crow::connections::systemBus, "xyz.openbmc_project.Settings",
        "/xyz/openbmc_project/time/sync_method",
        "xyz.openbmc_project.Time.Synchronization", "TimeSyncMethod",
        [asyncResp](const boost::system::error_code& errorCode,
                    const std::string& timeSyncMethod) {
        if (errorCode)
        {
            return;
        }

        if (timeSyncMethod ==
            "xyz.openbmc_project.Time.Synchronization.Method.NTP")
        {
            asyncResp->res.jsonValue["NTP"]["ProtocolEnabled"] = true;
        }
        else if (timeSyncMethod == "xyz.openbmc_project.Time.Synchronization."
                                   "Method.Manual")
        {
            asyncResp->res.jsonValue["NTP"]["ProtocolEnabled"] = false;
        }
        });
}

inline std::string encodeServiceObjectPath(std::string_view serviceName)
{
    sdbusplus::message::object_path objPath(
        "/xyz/openbmc_project/control/service");
    objPath /= serviceName;
    return objPath.str;
}

inline void handleBmcNetworkProtocolHead(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerNetworkProtocol/ManagerNetworkProtocol.json>; rel=describedby");
}

inline void handleManagersNetworkProtocolPatch(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<std::string> newHostName;
    std::optional<std::vector<nlohmann::json>> ntpServerObjects;
    std::optional<bool> ntpEnabled;
    std::optional<bool> ipmiEnabled;
    std::optional<bool> sshEnabled;

    // clang-format off
        if (!json_util::readJsonPatch(
                req, asyncResp->res,
                "HostName", newHostName,
                "NTP/NTPServers", ntpServerObjects,
                "NTP/ProtocolEnabled", ntpEnabled,
                "IPMI/ProtocolEnabled", ipmiEnabled,
                "SSH/ProtocolEnabled", sshEnabled))
        {
            return;
        }
    // clang-format on

    asyncResp->res.result(boost::beast::http::status::no_content);
    if (newHostName)
    {
        messages::propertyNotWritable(asyncResp->res, "HostName");
        return;
    }

    if (ntpEnabled)
    {
        handleNTPProtocolEnabled(*ntpEnabled, asyncResp);
    }
    if (ntpServerObjects)
    {
        getEthernetIfaceData(
            [asyncResp, ntpServerObjects](
                const bool success, std::vector<std::string>& currentNtpServers,
                const std::vector<std::string>& /*domainNames*/) {
            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            handleNTPServersPatch(asyncResp, *ntpServerObjects,
                                  std::move(currentNtpServers));
        });
    }

    if (ipmiEnabled)
    {
        handleProtocolEnabled(
            *ipmiEnabled, asyncResp,
            encodeServiceObjectPath(std::string(ipmiServiceName) + '@'));
    }

    if (sshEnabled)
    {
        handleProtocolEnabled(*sshEnabled, asyncResp,
                              encodeServiceObjectPath(sshServiceName));
    }
}

inline void handleManagersNetworkProtocolHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ManagerNetworkProtocol/ManagerNetworkProtocol.json>; rel=describedby");
}

inline void handleManagersNetworkProtocolGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    handleManagersNetworkProtocolHead(app, req, asyncResp);
    getNetworkData(asyncResp, req);
}

inline void requestRoutesNetworkProtocol(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/NetworkProtocol/")
        .privileges(redfish::privileges::patchManagerNetworkProtocol)
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleManagersNetworkProtocolPatch, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/NetworkProtocol/")
        .privileges(redfish::privileges::headManagerNetworkProtocol)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleManagersNetworkProtocolHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/NetworkProtocol/")
        .privileges(redfish::privileges::getManagerNetworkProtocol)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleManagersNetworkProtocolGet, std::ref(app)));
}

} // namespace redfish
