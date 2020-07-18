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

#include "error_messages.hpp"
#include "node.hpp"
#include "openbmc_dbus_rest.hpp"

#include <utils/json_utils.hpp>

#include <optional>
#include <variant>
namespace redfish
{

enum NetworkProtocolUnitStructFields
{
    NET_PROTO_UNIT_NAME,
    NET_PROTO_UNIT_DESC,
    NET_PROTO_UNIT_LOAD_STATE,
    NET_PROTO_UNIT_ACTIVE_STATE,
    NET_PROTO_UNIT_SUB_STATE,
    NET_PROTO_UNIT_DEVICE,
    NET_PROTO_UNIT_OBJ_PATH,
    NET_PROTO_UNIT_ALWAYS_0,
    NET_PROTO_UNIT_ALWAYS_EMPTY,
    NET_PROTO_UNIT_ALWAYS_ROOT_PATH
};

enum NetworkProtocolListenResponseElements
{
    NET_PROTO_LISTEN_TYPE,
    NET_PROTO_LISTEN_STREAM
};

/**
 * @brief D-Bus Unit structure returned in array from ListUnits Method
 */
using UnitStruct =
    std::tuple<std::string, std::string, std::string, std::string, std::string,
               std::string, sdbusplus::message::object_path, uint32_t,
               std::string, sdbusplus::message::object_path>;

const static std::array<std::pair<const char*, const char*>, 3> protocolToDBus{
    {{"SSH", "dropbear"}, {"HTTPS", "bmcweb"}, {"IPMI", "phosphor-ipmi-net"}}};

inline void
    extractNTPServersAndDomainNamesData(const GetManagedObjects& dbusData,
                                        std::vector<std::string>& ntpData,
                                        std::vector<std::string>& dnData)
{
    for (const auto& obj : dbusData)
    {
        for (const auto& ifacePair : obj.second)
        {
            if (obj.first == "/xyz/openbmc_project/network/eth0")
            {
                if (ifacePair.first ==
                    "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "NTPServers")
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
        }
    }
}

template <typename CallbackFunc>
void getEthernetIfaceData(CallbackFunc&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code errorCode,
            const GetManagedObjects& dbusData) {
            std::vector<std::string> ntpServers;
            std::vector<std::string> domainNames;

            if (errorCode)
            {
                callback(false, ntpServers, domainNames);
                return;
            }

            extractNTPServersAndDomainNamesData(dbusData, ntpServers,
                                                domainNames);

            callback(true, ntpServers, domainNames);
        },
        "xyz.openbmc_project.Network", "/xyz/openbmc_project/network",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

class NetworkProtocol : public Node
{
  public:
    NetworkProtocol(App& app) :
        Node(app, "/redfish/v1/Managers/bmc/NetworkProtocol/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        getData(asyncResp);
    }

    std::string getHostName() const
    {
        std::string hostName;

        std::array<char, HOST_NAME_MAX> hostNameCStr;
        if (gethostname(hostNameCStr.data(), hostNameCStr.size()) == 0)
        {
            hostName = hostNameCStr.data();
        }
        return hostName;
    }

    void getNTPProtocolEnabled(const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code errorCode,
                        const std::variant<std::string>& timeSyncMethod) {
                if (errorCode)
                {
                    return;
                }

                const std::string* s =
                    std::get_if<std::string>(&timeSyncMethod);

                if (*s == "xyz.openbmc_project.Time.Synchronization.Method.NTP")
                {
                    asyncResp->res.jsonValue["NTP"]["ProtocolEnabled"] = true;
                }
                else if (*s == "xyz.openbmc_project.Time.Synchronization."
                               "Method.Manual")
                {
                    asyncResp->res.jsonValue["NTP"]["ProtocolEnabled"] = false;
                }
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/time/sync_method",
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.Time.Synchronization", "TimeSyncMethod");
    }

    void getData(const std::shared_ptr<AsyncResp>& asyncResp)
    {
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
        asyncResp->res.jsonValue["HTTP"]["Port"] = 0;
        asyncResp->res.jsonValue["HTTP"]["ProtocolEnabled"] = false;

        for (auto& protocol : protocolToDBus)
        {
            asyncResp->res.jsonValue[protocol.first]["Port"] =
                nlohmann::detail::value_t::null;
            asyncResp->res.jsonValue[protocol.first]["ProtocolEnabled"] = false;
        }

        std::string hostName = getHostName();

        asyncResp->res.jsonValue["HostName"] = hostName;

        getNTPProtocolEnabled(asyncResp);

        // TODO Get eth0 interface data, and call the below callback for JSON
        // preparation
        getEthernetIfaceData(
            [hostName, asyncResp](const bool& success,
                                  const std::vector<std::string>& ntpServers,
                                  const std::vector<std::string>& domainNames) {
                if (!success)
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "EthernetInterface", "eth0");
                    return;
                }
                asyncResp->res.jsonValue["NTP"]["NTPServers"] = ntpServers;
                if (hostName.empty() == false)
                {
                    std::string fqdn = hostName;
                    if (domainNames.empty() == false)
                    {
                        fqdn += ".";
                        fqdn += domainNames[0];
                    }
                    asyncResp->res.jsonValue["FQDN"] = std::move(fqdn);
                }
            });

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code e,
                        const std::vector<UnitStruct>& r) {
                if (e)
                {
                    asyncResp->res.jsonValue = nlohmann::json::object();
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["HTTPS"]["Certificates"] = {
                    {"@odata.id", "/redfish/v1/Managers/bmc/NetworkProtocol/"
                                  "HTTPS/Certificates"}};

                for (auto& unit : r)
                {
                    /* Only traverse through <xyz>.socket units */
                    const std::string& unitName =
                        std::get<NET_PROTO_UNIT_NAME>(unit);
                    if (!boost::ends_with(unitName, ".socket"))
                    {
                        continue;
                    }

                    for (auto& kv : protocolToDBus)
                    {
                        // We are interested in services, which starts with
                        // mapped service name
                        if (!boost::starts_with(unitName, kv.second))
                        {
                            continue;
                        }
                        const char* rfServiceKey = kv.first;
                        const std::string& socketPath =
                            std::get<NET_PROTO_UNIT_OBJ_PATH>(unit);
                        const std::string& unitState =
                            std::get<NET_PROTO_UNIT_SUB_STATE>(unit);

                        asyncResp->res
                            .jsonValue[rfServiceKey]["ProtocolEnabled"] =
                            (unitState == "running") ||
                            (unitState == "listening");

                        crow::connections::systemBus->async_method_call(
                            [asyncResp,
                             rfServiceKey{std::string(rfServiceKey)}](
                                const boost::system::error_code ec,
                                const std::variant<std::vector<std::tuple<
                                    std::string, std::string>>>& resp) {
                                if (ec)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                const std::vector<
                                    std::tuple<std::string, std::string>>*
                                    responsePtr = std::get_if<std::vector<
                                        std::tuple<std::string, std::string>>>(
                                        &resp);
                                if (responsePtr == nullptr ||
                                    responsePtr->size() < 1)
                                {
                                    return;
                                }

                                const std::string& listenStream =
                                    std::get<NET_PROTO_LISTEN_STREAM>(
                                        (*responsePtr)[0]);
                                std::size_t lastColonPos =
                                    listenStream.rfind(':');
                                if (lastColonPos == std::string::npos)
                                {
                                    // Not a port
                                    return;
                                }
                                std::string portStr =
                                    listenStream.substr(lastColonPos + 1);
                                if (portStr.empty())
                                {
                                    return;
                                }
                                char* endPtr = nullptr;
                                errno = 0;
                                // Use strtol instead of stroi to avoid
                                // exceptions
                                long port =
                                    std::strtol(portStr.c_str(), &endPtr, 10);
                                if ((errno == 0) && (*endPtr == '\0'))
                                {
                                    asyncResp->res
                                        .jsonValue[rfServiceKey]["Port"] = port;
                                }
                                return;
                            },
                            "org.freedesktop.systemd1", socketPath,
                            "org.freedesktop.DBus.Properties", "Get",
                            "org.freedesktop.systemd1.Socket", "Listen");

                        // We found service, break the inner loop.
                        break;
                    }
                }
            },
            "org.freedesktop.systemd1", "/org/freedesktop/systemd1",
            "org.freedesktop.systemd1.Manager", "ListUnits");
    }

    void handleHostnamePatch(const std::string& hostName,
                             const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network",
            "/xyz/openbmc_project/network/config",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.SystemConfiguration", "HostName",
            std::variant<std::string>(hostName));
    }

    void handleNTPProtocolEnabled(const bool& ntpEnabled,
                                  const std::shared_ptr<AsyncResp>& asyncResp)
    {
        std::string timeSyncMethod;
        if (ntpEnabled)
        {
            timeSyncMethod =
                "xyz.openbmc_project.Time.Synchronization.Method.NTP";
        }
        else
        {
            timeSyncMethod =
                "xyz.openbmc_project.Time.Synchronization.Method.Manual";
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code errorCode) {
                if (errorCode)
                {
                    messages::internalError(asyncResp->res);
                }
            },
            "xyz.openbmc_project.Settings",
            "/xyz/openbmc_project/time/sync_method",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Time.Synchronization", "TimeSyncMethod",
            std::variant<std::string>{timeSyncMethod});
    }

    void handleNTPServersPatch(const std::vector<std::string>& ntpServers,
                               const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "xyz.openbmc_project.Network", "/xyz/openbmc_project/network/eth0",
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Network.EthernetInterface", "NTPServers",
            std::variant<std::vector<std::string>>{ntpServers});
    }

    void handleIpmiProtocolEnabled(const bool ipmiProtocolEnabled,
                                   const std::shared_ptr<AsyncResp>& asyncResp)
    {
        crow::connections::systemBus->async_method_call(
            [ipmiProtocolEnabled,
             asyncResp](const boost::system::error_code ec,
                        const crow::openbmc_mapper::GetSubTreeType& subtree) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                constexpr char const* netipmidBasePath =
                    "/xyz/openbmc_project/control/service/"
                    "phosphor_2dipmi_2dnet_40";

                for (const auto& entry : subtree)
                {
                    if (boost::algorithm::starts_with(entry.first,
                                                      netipmidBasePath))
                    {
                        crow::connections::systemBus->async_method_call(
                            [asyncResp](const boost::system::error_code ec2) {
                                if (ec2)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            },
                            entry.second.begin()->first, entry.first,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Control.Service.Attributes",
                            "Running", std::variant<bool>{ipmiProtocolEnabled});

                        crow::connections::systemBus->async_method_call(
                            [asyncResp](const boost::system::error_code ec2) {
                                if (ec2)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                            },
                            entry.second.begin()->first, entry.first,
                            "org.freedesktop.DBus.Properties", "Set",
                            "xyz.openbmc_project.Control.Service.Attributes",
                            "Enabled", std::variant<bool>{ipmiProtocolEnabled});
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/control/service", 0,
            std::array<const char*, 1>{
                "xyz.openbmc_project.Control.Service.Attributes"});
    }

    void doPatch(crow::Response& res, const crow::Request& req,
                 const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        std::optional<std::string> newHostName;
        std::optional<nlohmann::json> ntp;
        std::optional<nlohmann::json> ipmi;

        if (!json_util::readJson(req, res, "HostName", newHostName, "NTP", ntp,
                                 "IPMI", ipmi))
        {
            return;
        }

        res.result(boost::beast::http::status::no_content);
        if (newHostName)
        {
            handleHostnamePatch(*newHostName, asyncResp);
        }

        if (ntp)
        {
            std::optional<std::vector<std::string>> ntpServers;
            std::optional<bool> ntpEnabled;
            if (!json_util::readJson(*ntp, res, "NTPServers", ntpServers,
                                     "ProtocolEnabled", ntpEnabled))
            {
                return;
            }

            if (ntpEnabled)
            {
                handleNTPProtocolEnabled(*ntpEnabled, asyncResp);
            }

            if (ntpServers)
            {
                std::sort((*ntpServers).begin(), (*ntpServers).end());
                (*ntpServers)
                    .erase(
                        std::unique((*ntpServers).begin(), (*ntpServers).end()),
                        (*ntpServers).end());
                handleNTPServersPatch(*ntpServers, asyncResp);
            }
        }

        if (ipmi)
        {
            std::optional<bool> ipmiProtocolEnabled;
            if (!json_util::readJson(*ipmi, res, "ProtocolEnabled",
                                     ipmiProtocolEnabled))
            {
                return;
            }

            if (ipmiProtocolEnabled)
            {
                handleIpmiProtocolEnabled(*ipmiProtocolEnabled, asyncResp);
            }
        }
    }
};

} // namespace redfish
