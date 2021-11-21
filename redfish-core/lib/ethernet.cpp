#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "../../http/app_class_decl.hpp"
using crow::App;
#include "ethernet.hpp"

namespace redfish
{

bool extractEthernetInterfaceData(const std::string& ethifaceId,
                                         GetManagedObjects& dbusData,
                                         EthernetInterfaceData& ethData)
{
    bool idFound = false;
    for (auto& objpath : dbusData)
    {
        for (auto& ifacePair : objpath.second)
        {
            if (objpath.first == "/xyz/openbmc_project/network/" + ethifaceId)
            {
                idFound = true;
                if (ifacePair.first == "xyz.openbmc_project.Network.MACAddress")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "MACAddress")
                        {
                            const std::string* mac =
                                std::get_if<std::string>(&propertyPair.second);
                            if (mac != nullptr)
                            {
                                ethData.mac_address = *mac;
                            }
                        }
                    }
                }
                else if (ifacePair.first == "xyz.openbmc_project.Network.VLAN")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "Id")
                        {
                            const uint32_t* id =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (id != nullptr)
                            {
                                ethData.vlan_id.push_back(*id);
                            }
                        }
                    }
                }
                else if (ifacePair.first ==
                         "xyz.openbmc_project.Network.EthernetInterface")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "AutoNeg")
                        {
                            const bool* autoNeg =
                                std::get_if<bool>(&propertyPair.second);
                            if (autoNeg != nullptr)
                            {
                                ethData.auto_neg = *autoNeg;
                            }
                        }
                        else if (propertyPair.first == "Speed")
                        {
                            const uint32_t* speed =
                                std::get_if<uint32_t>(&propertyPair.second);
                            if (speed != nullptr)
                            {
                                ethData.speed = *speed;
                            }
                        }
                        else if (propertyPair.first == "LinkUp")
                        {
                            const bool* linkUp =
                                std::get_if<bool>(&propertyPair.second);
                            if (linkUp != nullptr)
                            {
                                ethData.linkUp = *linkUp;
                            }
                        }
                        else if (propertyPair.first == "NICEnabled")
                        {
                            const bool* nicEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (nicEnabled != nullptr)
                            {
                                ethData.nicEnabled = *nicEnabled;
                            }
                        }
                        else if (propertyPair.first == "Nameservers")
                        {
                            const std::vector<std::string>* nameservers =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (nameservers != nullptr)
                            {
                                ethData.nameServers = *nameservers;
                            }
                        }
                        else if (propertyPair.first == "StaticNameServers")
                        {
                            const std::vector<std::string>* staticNameServers =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (staticNameServers != nullptr)
                            {
                                ethData.staticNameServers = *staticNameServers;
                            }
                        }
                        else if (propertyPair.first == "DHCPEnabled")
                        {
                            const std::string* dhcpEnabled =
                                std::get_if<std::string>(&propertyPair.second);
                            if (dhcpEnabled != nullptr)
                            {
                                ethData.DHCPEnabled = *dhcpEnabled;
                            }
                        }
                        else if (propertyPair.first == "DomainName")
                        {
                            const std::vector<std::string>* domainNames =
                                std::get_if<std::vector<std::string>>(
                                    &propertyPair.second);
                            if (domainNames != nullptr)
                            {
                                ethData.domainnames = *domainNames;
                            }
                        }
                        else if (propertyPair.first == "DefaultGateway")
                        {
                            const std::string* defaultGateway =
                                std::get_if<std::string>(&propertyPair.second);
                            if (defaultGateway != nullptr)
                            {
                                std::string defaultGatewayStr = *defaultGateway;
                                if (defaultGatewayStr.empty())
                                {
                                    ethData.default_gateway = "0.0.0.0";
                                }
                                else
                                {
                                    ethData.default_gateway = defaultGatewayStr;
                                }
                            }
                        }
                        else if (propertyPair.first == "DefaultGateway6")
                        {
                            const std::string* defaultGateway6 =
                                std::get_if<std::string>(&propertyPair.second);
                            if (defaultGateway6 != nullptr)
                            {
                                std::string defaultGateway6Str =
                                    *defaultGateway6;
                                if (defaultGateway6Str.empty())
                                {
                                    ethData.ipv6_default_gateway =
                                        "0:0:0:0:0:0:0:0";
                                }
                                else
                                {
                                    ethData.ipv6_default_gateway =
                                        defaultGateway6Str;
                                }
                            }
                        }
                    }
                }
            }

            if (objpath.first == "/xyz/openbmc_project/network/config/dhcp")
            {
                if (ifacePair.first ==
                    "xyz.openbmc_project.Network.DHCPConfiguration")
                {
                    for (const auto& propertyPair : ifacePair.second)
                    {
                        if (propertyPair.first == "DNSEnabled")
                        {
                            const bool* dnsEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (dnsEnabled != nullptr)
                            {
                                ethData.DNSEnabled = *dnsEnabled;
                            }
                        }
                        else if (propertyPair.first == "NTPEnabled")
                        {
                            const bool* ntpEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (ntpEnabled != nullptr)
                            {
                                ethData.NTPEnabled = *ntpEnabled;
                            }
                        }
                        else if (propertyPair.first == "HostNameEnabled")
                        {
                            const bool* hostNameEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (hostNameEnabled != nullptr)
                            {
                                ethData.HostNameEnabled = *hostNameEnabled;
                            }
                        }
                        else if (propertyPair.first == "SendHostNameEnabled")
                        {
                            const bool* sendHostNameEnabled =
                                std::get_if<bool>(&propertyPair.second);
                            if (sendHostNameEnabled != nullptr)
                            {
                                ethData.SendHostNameEnabled =
                                    *sendHostNameEnabled;
                            }
                        }
                    }
                }
            }
            // System configuration shows up in the global namespace, so no need
            // to check eth number
            if (ifacePair.first ==
                "xyz.openbmc_project.Network.SystemConfiguration")
            {
                for (const auto& propertyPair : ifacePair.second)
                {
                    if (propertyPair.first == "HostName")
                    {
                        const std::string* hostname =
                            std::get_if<std::string>(&propertyPair.second);
                        if (hostname != nullptr)
                        {
                            ethData.hostname = *hostname;
                        }
                    }
                }
            }
        }
    }
    return idFound;
}

void extractIPV6Data(const std::string& ethifaceId,
                    const GetManagedObjects& dbusData,
                    boost::container::flat_set<IPv6AddressData>& ipv6Config)
{
    const std::string ipv6PathStart =
        "/xyz/openbmc_project/network/" + ethifaceId + "/ipv6/";

    // Since there might be several IPv6 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto& objpath : dbusData)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv6PathStart))
        {
            for (auto& interface : objpath.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv6AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv6AddressData>::iterator,
                        bool>
                        it = ipv6Config.insert(IPv6AddressData{});
                    IPv6AddressData& ipv6Address = *it.first;
                    ipv6Address.id =
                        objpath.first.str.substr(ipv6PathStart.size());
                    for (auto& property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string* address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv6Address.address = *address;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string* origin =
                                std::get_if<std::string>(&property.second);
                            if (origin != nullptr)
                            {
                                ipv6Address.origin =
                                    translateAddressOriginDbusToRedfish(*origin,
                                                                        false);
                            }
                        }
                        else if (property.first == "PrefixLength")
                        {
                            const uint8_t* prefix =
                                std::get_if<uint8_t>(&property.second);
                            if (prefix != nullptr)
                            {
                                ipv6Address.prefixLength = *prefix;
                            }
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR
                                << "Got extra property: " << property.first
                                << " on the " << objpath.first.str << " object";
                        }
                    }
                }
            }
        }
    }
}

void
    extractIPData(const std::string& ethifaceId,
                  const GetManagedObjects& dbusData,
                  boost::container::flat_set<IPv4AddressData>& ipv4Config)
{
    const std::string ipv4PathStart =
        "/xyz/openbmc_project/network/" + ethifaceId + "/ipv4/";

    // Since there might be several IPv4 configurations aligned with
    // single ethernet interface, loop over all of them
    for (const auto& objpath : dbusData)
    {
        // Check if proper pattern for object path appears
        if (boost::starts_with(objpath.first.str, ipv4PathStart))
        {
            for (auto& interface : objpath.second)
            {
                if (interface.first == "xyz.openbmc_project.Network.IP")
                {
                    // Instance IPv4AddressData structure, and set as
                    // appropriate
                    std::pair<
                        boost::container::flat_set<IPv4AddressData>::iterator,
                        bool>
                        it = ipv4Config.insert(IPv4AddressData{});
                    IPv4AddressData& ipv4Address = *it.first;
                    ipv4Address.id =
                        objpath.first.str.substr(ipv4PathStart.size());
                    for (auto& property : interface.second)
                    {
                        if (property.first == "Address")
                        {
                            const std::string* address =
                                std::get_if<std::string>(&property.second);
                            if (address != nullptr)
                            {
                                ipv4Address.address = *address;
                            }
                        }
                        else if (property.first == "Origin")
                        {
                            const std::string* origin =
                                std::get_if<std::string>(&property.second);
                            if (origin != nullptr)
                            {
                                ipv4Address.origin =
                                    translateAddressOriginDbusToRedfish(*origin,
                                                                        true);
                            }
                        }
                        else if (property.first == "PrefixLength")
                        {
                            const uint8_t* mask =
                                std::get_if<uint8_t>(&property.second);
                            if (mask != nullptr)
                            {
                                // convert it to the string
                                ipv4Address.netmask = getNetmask(*mask);
                            }
                        }
                        else
                        {
                            BMCWEB_LOG_ERROR
                                << "Got extra property: " << property.first
                                << " on the " << objpath.first.str << " object";
                        }
                    }
                    // Check if given address is local, or global
                    ipv4Address.linktype =
                        boost::starts_with(ipv4Address.address, "169.254.")
                            ? LinkType::Local
                            : LinkType::Global;
                }
            }
        }
    }
}

void requestEthernetInterfacesRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/")
        .privileges(redfish::privileges::getEthernetInterfaceCollection)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request&,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            asyncResp->res.jsonValue["@odata.type"] =
                "#EthernetInterfaceCollection.EthernetInterfaceCollection";
            asyncResp->res.jsonValue["@odata.id"] =
                "/redfish/v1/Managers/bmc/EthernetInterfaces";
            asyncResp->res.jsonValue["Name"] =
                "Ethernet Network Interface Collection";
            asyncResp->res.jsonValue["Description"] =
                "Collection of EthernetInterfaces for this Manager";

            // Get eth interface list, and call the below callback for JSON
            // preparation
            getEthernetIfaceList([asyncResp](const bool& success,
                                             const boost::container::flat_set<
                                                 std::string>& ifaceList) {
                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& ifaceArray =
                    asyncResp->res.jsonValue["Members"];
                ifaceArray = nlohmann::json::array();
                std::string tag = "_";
                for (const std::string& ifaceItem : ifaceList)
                {
                    std::size_t found = ifaceItem.find(tag);
                    if (found == std::string::npos)
                    {
                        ifaceArray.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                                  ifaceItem}});
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces";
            });
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::getEthernetInterface)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& ifaceId) {
                getEthernetIfaceData(
                    ifaceId,
                    [asyncResp,
                     ifaceId](const bool& success,
                              const EthernetInterfaceData& ethData,
                              const boost::container::flat_set<IPv4AddressData>&
                                  ipv4Data,
                              const boost::container::flat_set<IPv6AddressData>&
                                  ipv6Data) {
                        if (!success)
                        {
                            // TODO(Pawel)consider distinguish between non
                            // existing object, and other errors
                            messages::resourceNotFound(
                                asyncResp->res, "EthernetInterface", ifaceId);
                            return;
                        }

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#EthernetInterface.v1_4_1.EthernetInterface";
                        asyncResp->res.jsonValue["Name"] =
                            "Manager Ethernet Interface";
                        asyncResp->res.jsonValue["Description"] =
                            "Management Network Interface";

                        parseInterfaceData(asyncResp, ifaceId, ethData,
                                           ipv4Data, ipv6Data);
                    });
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::patchEthernetInterface)

        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& ifaceId) {
                std::optional<std::string> hostname;
                std::optional<std::string> fqdn;
                std::optional<std::string> macAddress;
                std::optional<std::string> ipv6DefaultGateway;
                std::optional<nlohmann::json> ipv4StaticAddresses;
                std::optional<nlohmann::json> ipv6StaticAddresses;
                std::optional<std::vector<std::string>> staticNameServers;
                std::optional<nlohmann::json> dhcpv4;
                std::optional<nlohmann::json> dhcpv6;
                std::optional<bool> interfaceEnabled;
                DHCPParameters v4dhcpParms;
                DHCPParameters v6dhcpParms;

                if (!json_util::readJson(
                        req, asyncResp->res, "HostName", hostname, "FQDN", fqdn,
                        "IPv4StaticAddresses", ipv4StaticAddresses,
                        "MACAddress", macAddress, "StaticNameServers",
                        staticNameServers, "IPv6DefaultGateway",
                        ipv6DefaultGateway, "IPv6StaticAddresses",
                        ipv6StaticAddresses, "DHCPv4", dhcpv4, "DHCPv6", dhcpv6,
                        "InterfaceEnabled", interfaceEnabled))
                {
                    return;
                }
                if (dhcpv4)
                {
                    if (!json_util::readJson(
                            *dhcpv4, asyncResp->res, "DHCPEnabled",
                            v4dhcpParms.dhcpv4Enabled, "UseDNSServers",
                            v4dhcpParms.useDNSServers, "UseNTPServers",
                            v4dhcpParms.useNTPServers, "UseDomainName",
                            v4dhcpParms.useUseDomainName))
                    {
                        return;
                    }
                }

                if (dhcpv6)
                {
                    if (!json_util::readJson(
                            *dhcpv6, asyncResp->res, "OperatingMode",
                            v6dhcpParms.dhcpv6OperatingMode, "UseDNSServers",
                            v6dhcpParms.useDNSServers, "UseNTPServers",
                            v6dhcpParms.useNTPServers, "UseDomainName",
                            v6dhcpParms.useUseDomainName))
                    {
                        return;
                    }
                }

                // Get single eth interface data, and call the below callback
                // for JSON preparation
                getEthernetIfaceData(
                    ifaceId,
                    [asyncResp, ifaceId, hostname = std::move(hostname),
                     fqdn = std::move(fqdn), macAddress = std::move(macAddress),
                     ipv4StaticAddresses = std::move(ipv4StaticAddresses),
                     ipv6DefaultGateway = std::move(ipv6DefaultGateway),
                     ipv6StaticAddresses = std::move(ipv6StaticAddresses),
                     staticNameServers = std::move(staticNameServers),
                     dhcpv4 = std::move(dhcpv4), dhcpv6 = std::move(dhcpv6),
                     v4dhcpParms = std::move(v4dhcpParms),
                     v6dhcpParms = std::move(v6dhcpParms), interfaceEnabled](
                        const bool& success,
                        const EthernetInterfaceData& ethData,
                        const boost::container::flat_set<IPv4AddressData>&
                            ipv4Data,
                        const boost::container::flat_set<IPv6AddressData>&
                            ipv6Data) {
                        if (!success)
                        {
                            // ... otherwise return error
                            // TODO(Pawel)consider distinguish between non
                            // existing object, and other errors
                            messages::resourceNotFound(
                                asyncResp->res, "Ethernet Interface", ifaceId);
                            return;
                        }

                        if (dhcpv4 || dhcpv6)
                        {
                            handleDHCPPatch(ifaceId, ethData, v4dhcpParms,
                                            v6dhcpParms, asyncResp);
                        }

                        if (hostname)
                        {
                            handleHostnamePatch(*hostname, asyncResp);
                        }

                        if (fqdn)
                        {
                            handleFqdnPatch(ifaceId, *fqdn, asyncResp);
                        }

                        if (macAddress)
                        {
                            handleMACAddressPatch(ifaceId, *macAddress,
                                                  asyncResp);
                        }

                        if (ipv4StaticAddresses)
                        {
                            // TODO(ed) for some reason the capture of
                            // ipv4Addresses above is returning a const value,
                            // not a non-const value. This doesn't really work
                            // for us, as we need to be able to efficiently move
                            // out the intermedia nlohmann::json objects. This
                            // makes a copy of the structure, and operates on
                            // that, but could be done more efficiently
                            nlohmann::json ipv4Static = *ipv4StaticAddresses;
                            handleIPv4StaticPatch(ifaceId, ipv4Static, ipv4Data,
                                                  asyncResp);
                        }

                        if (staticNameServers)
                        {
                            handleStaticNameServersPatch(
                                ifaceId, *staticNameServers, asyncResp);
                        }

                        if (ipv6DefaultGateway)
                        {
                            messages::propertyNotWritable(asyncResp->res,
                                                          "IPv6DefaultGateway");
                        }

                        if (ipv6StaticAddresses)
                        {
                            nlohmann::json ipv6Static = *ipv6StaticAddresses;
                            handleIPv6StaticAddressesPatch(ifaceId, ipv6Static,
                                                           ipv6Data, asyncResp);
                        }

                        if (interfaceEnabled)
                        {
                            setEthernetInterfaceBoolProperty(
                                ifaceId, "NICEnabled", *interfaceEnabled,
                                asyncResp);
                        }
                    });
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/")
        .privileges(redfish::privileges::getVLanNetworkInterface)

        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /* req */,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& parentIfaceId, const std::string& ifaceId) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#VLanNetworkInterface.v1_1_0.VLanNetworkInterface";
                asyncResp->res.jsonValue["Name"] = "VLAN Network Interface";

                if (!verifyNames(parentIfaceId, ifaceId))
                {
                    return;
                }

                // Get single eth interface data, and call the below callback
                // for JSON preparation
                getEthernetIfaceData(
                    ifaceId,
                    [asyncResp, parentIfaceId, ifaceId](
                        const bool& success,
                        const EthernetInterfaceData& ethData,
                        const boost::container::flat_set<IPv4AddressData>&,
                        const boost::container::flat_set<IPv6AddressData>&) {
                        if (success && ethData.vlan_id.size() != 0)
                        {
                            parseInterfaceData(asyncResp->res.jsonValue,
                                               parentIfaceId, ifaceId, ethData);
                        }
                        else
                        {
                            // ... otherwise return error
                            // TODO(Pawel)consider distinguish between non
                            // existing object, and other errors
                            messages::resourceNotFound(asyncResp->res,
                                                       "VLAN Network Interface",
                                                       ifaceId);
                        }
                    });
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/")
        // This privilege is incorrect, it should be ConfigureManager
        //.privileges(redfish::privileges::patchVLanNetworkInterface)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::patch)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& parentIfaceId, const std::string& ifaceId) {
                if (!verifyNames(parentIfaceId, ifaceId))
                {
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", ifaceId);
                    return;
                }

                bool vlanEnable = false;
                uint32_t vlanId = 0;

                if (!json_util::readJson(req, asyncResp->res, "VLANEnable",
                                         vlanEnable, "VLANId", vlanId))
                {
                    return;
                }

                // Get single eth interface data, and call the below callback
                // for JSON preparation
                getEthernetIfaceData(
                    ifaceId,
                    [asyncResp, parentIfaceId, ifaceId, &vlanEnable, &vlanId](
                        const bool& success,
                        const EthernetInterfaceData& ethData,
                        const boost::container::flat_set<IPv4AddressData>&,
                        const boost::container::flat_set<IPv6AddressData>&) {
                        if (success && !ethData.vlan_id.empty())
                        {
                            auto callback =
                                [asyncResp](
                                    const boost::system::error_code ec) {
                                    if (ec)
                                    {
                                        messages::internalError(asyncResp->res);
                                    }
                                };

                            if (vlanEnable == true)
                            {
                                crow::connections::systemBus->async_method_call(
                                    std::move(callback),
                                    "xyz.openbmc_project.Network",
                                    "/xyz/openbmc_project/network/" + ifaceId,
                                    "org.freedesktop.DBus.Properties", "Set",
                                    "xyz.openbmc_project.Network.VLAN", "Id",
                                    std::variant<uint32_t>(vlanId));
                            }
                            else
                            {
                                BMCWEB_LOG_DEBUG
                                    << "vlanEnable is false. Deleting the "
                                       "vlan interface";
                                crow::connections::systemBus->async_method_call(
                                    std::move(callback),
                                    "xyz.openbmc_project.Network",
                                    std::string(
                                        "/xyz/openbmc_project/network/") +
                                        ifaceId,
                                    "xyz.openbmc_project.Object.Delete",
                                    "Delete");
                            }
                        }
                        else
                        {
                            // TODO(Pawel)consider distinguish between non
                            // existing object, and other errors
                            messages::resourceNotFound(asyncResp->res,
                                                       "VLAN Network Interface",
                                                       ifaceId);
                            return;
                        }
                    });
            });

    BMCWEB_ROUTE(
        app, "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/<str>/")
        // This privilege is incorrect, it should be ConfigureManager
        //.privileges(redfish::privileges::deleteVLanNetworkInterface)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::delete_)(
            [](const crow::Request& /* req */,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& parentIfaceId, const std::string& ifaceId) {
                if (!verifyNames(parentIfaceId, ifaceId))
                {
                    messages::resourceNotFound(
                        asyncResp->res, "VLAN Network Interface", ifaceId);
                    return;
                }

                // Get single eth interface data, and call the below callback
                // for JSON preparation
                getEthernetIfaceData(
                    ifaceId,
                    [asyncResp, parentIfaceId, ifaceId](
                        const bool& success,
                        const EthernetInterfaceData& ethData,
                        const boost::container::flat_set<IPv4AddressData>&,
                        const boost::container::flat_set<IPv6AddressData>&) {
                        if (success && !ethData.vlan_id.empty())
                        {
                            auto callback =
                                [asyncResp](
                                    const boost::system::error_code ec) {
                                    if (ec)
                                    {
                                        messages::internalError(asyncResp->res);
                                    }
                                };
                            crow::connections::systemBus->async_method_call(
                                std::move(callback),
                                "xyz.openbmc_project.Network",
                                std::string("/xyz/openbmc_project/network/") +
                                    ifaceId,
                                "xyz.openbmc_project.Object.Delete", "Delete");
                        }
                        else
                        {
                            // ... otherwise return error
                            // TODO(Pawel)consider distinguish between non
                            // existing object, and other errors
                            messages::resourceNotFound(asyncResp->res,
                                                       "VLAN Network Interface",
                                                       ifaceId);
                        }
                    });
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/")

        .privileges(redfish::privileges::getVLanNetworkInterfaceCollection)
        .methods(
            boost::beast::http::verb::
                get)([](const crow::Request& /* req */,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& rootInterfaceName) {
            // Get eth interface list, and call the below callback for JSON
            // preparation
            getEthernetIfaceList([asyncResp, rootInterfaceName](
                                     const bool& success,
                                     const boost::container::flat_set<
                                         std::string>& ifaceList) {
                if (!success)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                if (ifaceList.find(rootInterfaceName) == ifaceList.end())
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "VLanNetworkInterfaceCollection",
                                               rootInterfaceName);
                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#VLanNetworkInterfaceCollection."
                    "VLanNetworkInterfaceCollection";
                asyncResp->res.jsonValue["Name"] =
                    "VLAN Network Interface Collection";

                nlohmann::json ifaceArray = nlohmann::json::array();

                for (const std::string& ifaceItem : ifaceList)
                {
                    if (boost::starts_with(ifaceItem, rootInterfaceName + "_"))
                    {
                        std::string path =
                            "/redfish/v1/Managers/bmc/EthernetInterfaces/";
                        path += rootInterfaceName;
                        path += "/VLANs/";
                        path += ifaceItem;
                        ifaceArray.push_back({{"@odata.id", std::move(path)}});
                    }
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    ifaceArray.size();
                asyncResp->res.jsonValue["Members"] = std::move(ifaceArray);
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/EthernetInterfaces/" +
                    rootInterfaceName + "/VLANs";
            });
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Managers/bmc/EthernetInterfaces/<str>/VLANs/")
        // This privilege is wrong, it should be ConfigureManager
        //.privileges(redfish::privileges::postVLanNetworkInterfaceCollection)
        .privileges({{"ConfigureComponents"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& rootInterfaceName) {
                bool vlanEnable = false;
                uint32_t vlanId = 0;
                if (!json_util::readJson(req, asyncResp->res, "VLANId", vlanId,
                                         "VLANEnable", vlanEnable))
                {
                    return;
                }
                // Need both vlanId and vlanEnable to service this request
                if (!vlanId)
                {
                    messages::propertyMissing(asyncResp->res, "VLANId");
                }
                if (!vlanEnable)
                {
                    messages::propertyMissing(asyncResp->res, "VLANEnable");
                }
                if (static_cast<bool>(vlanId) ^ vlanEnable)
                {
                    return;
                }

                auto callback =
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            // TODO(ed) make more consistent error messages
                            // based on phosphor-network responses
                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::created(asyncResp->res);
                    };
                crow::connections::systemBus->async_method_call(
                    std::move(callback), "xyz.openbmc_project.Network",
                    "/xyz/openbmc_project/network",
                    "xyz.openbmc_project.Network.VLAN.Create", "VLAN",
                    rootInterfaceName, vlanId);
            });
}

}

#include "hypervisor_system.hpp"
#include "network_protocol.hpp"

namespace redfish::hypervisor
{

void requestRoutesHypervisorSystems(App& app)
{
    /**
     * Hypervisor Systems derived class for delivering Computer Systems Schema.
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/")
        .privileges(redfish::privileges::getComputerSystem)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code ec,
                                const std::variant<std::string>& /*hostName*/) {
                        if (ec)
                        {
                            messages::resourceNotFound(asyncResp->res, "System",
                                                       "hypervisor");
                            return;
                        }
                        BMCWEB_LOG_DEBUG << "Hypervisor is available";

                        asyncResp->res.jsonValue["@odata.type"] =
                            "#ComputerSystem.v1_6_0.ComputerSystem";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Systems/hypervisor";
                        asyncResp->res.jsonValue["Description"] = "Hypervisor";
                        asyncResp->res.jsonValue["Name"] = "Hypervisor";
                        asyncResp->res.jsonValue["Id"] = "hypervisor";
                        asyncResp->res.jsonValue["SystemType"] = "OS";
                        asyncResp->res.jsonValue["Links"]["ManagedBy"] = {
                            {{"@odata.id", "/redfish/v1/Managers/bmc"}}};
                        asyncResp->res.jsonValue["EthernetInterfaces"] = {
                            {"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                          "EthernetInterfaces"}};
                        getHypervisorState(asyncResp);
                        getHypervisorActions(asyncResp);
                        // TODO: Add "SystemType" : "hypervisor"
                    },
                    "xyz.openbmc_project.Settings",
                    "/xyz/openbmc_project/network/hypervisor",
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Network.SystemConfiguration",
                    "HostName");
            });

    /**
     * HypervisorInterfaceCollection class to handle the GET and PATCH on
     * Hypervisor Interface
     */

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/EthernetInterfaces/")
        .privileges(redfish::privileges::getEthernetInterfaceCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                const std::array<const char*, 1> interfaces = {
                    "xyz.openbmc_project.Network.EthernetInterface"};

                crow::connections::systemBus->async_method_call(
                    [asyncResp](const boost::system::error_code error,
                                const std::vector<std::string>& ifaceList) {
                        if (error)
                        {
                            messages::resourceNotFound(asyncResp->res, "System",
                                                       "hypervisor");
                            return;
                        }
                        asyncResp->res.jsonValue["@odata.type"] =
                            "#EthernetInterfaceCollection."
                            "EthernetInterfaceCollection";
                        asyncResp->res.jsonValue["@odata.id"] =
                            "/redfish/v1/Systems/hypervisor/EthernetInterfaces";
                        asyncResp->res.jsonValue["Name"] =
                            "Hypervisor Ethernet "
                            "Interface Collection";
                        asyncResp->res.jsonValue["Description"] =
                            "Collection of Virtual Management "
                            "Interfaces for the hypervisor";

                        nlohmann::json& ifaceArray =
                            asyncResp->res.jsonValue["Members"];
                        ifaceArray = nlohmann::json::array();
                        for (const std::string& iface : ifaceList)
                        {
                            sdbusplus::message::object_path path(iface);
                            std::string name = path.filename();
                            if (name.empty())
                            {
                                continue;
                            }

                            ifaceArray.push_back(
                                {{"@odata.id", "/redfish/v1/Systems/hypervisor/"
                                               "EthernetInterfaces/" +
                                                   name}});
                        }
                        asyncResp->res.jsonValue["Members@odata.count"] =
                            ifaceArray.size();
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
                    "/xyz/openbmc_project/network/hypervisor", 0, interfaces);
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::getEthernetInterface)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& id) {
            getHypervisorIfaceData(
                id,
                [asyncResp, ifaceId{std::string(id)}](
                    const bool& success, const EthernetInterfaceData& ethData,
                    const boost::container::flat_set<IPv4AddressData>&
                        ipv4Data) {
                    if (!success)
                    {
                        messages::resourceNotFound(
                            asyncResp->res, "EthernetInterface", ifaceId);
                        return;
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#EthernetInterface.v1_5_1.EthernetInterface";
                    asyncResp->res.jsonValue["Name"] =
                        "Hypervisor Ethernet Interface";
                    asyncResp->res.jsonValue["Description"] =
                        "Hypervisor's Virtual Management Ethernet Interface";
                    parseInterfaceData(asyncResp->res.jsonValue, ifaceId,
                                       ethData, ipv4Data);
                });
        });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/EthernetInterfaces/<str>/")
        .privileges(redfish::privileges::patchEthernetInterface)
        .methods(
            boost::beast::http::verb::
                patch)([](const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& ifaceId) {
            std::optional<std::string> hostName;
            std::optional<std::vector<nlohmann::json>> ipv4StaticAddresses;
            std::optional<nlohmann::json> ipv4Addresses;
            std::optional<nlohmann::json> dhcpv4;
            std::optional<bool> ipv4DHCPEnabled;

            if (!json_util::readJson(req, asyncResp->res, "HostName", hostName,
                                     "IPv4StaticAddresses", ipv4StaticAddresses,
                                     "IPv4Addresses", ipv4Addresses, "DHCPv4",
                                     dhcpv4))
            {
                return;
            }

            if (ipv4Addresses)
            {
                messages::propertyNotWritable(asyncResp->res, "IPv4Addresses");
                return;
            }

            if (dhcpv4)
            {
                if (!json_util::readJson(*dhcpv4, asyncResp->res, "DHCPEnabled",
                                         ipv4DHCPEnabled))
                {
                    return;
                }
            }

            getHypervisorIfaceData(
                ifaceId,
                [asyncResp, ifaceId, hostName = std::move(hostName),
                 ipv4StaticAddresses = std::move(ipv4StaticAddresses),
                 ipv4DHCPEnabled, dhcpv4 = std::move(dhcpv4)](
                    const bool& success, const EthernetInterfaceData& ethData,
                    const boost::container::flat_set<IPv4AddressData>&) {
                    if (!success)
                    {
                        messages::resourceNotFound(
                            asyncResp->res, "EthernetInterface", ifaceId);
                        return;
                    }

                    if (ipv4StaticAddresses)
                    {
                        const nlohmann::json& ipv4Static = *ipv4StaticAddresses;
                        if (ipv4Static.begin() == ipv4Static.end())
                        {
                            messages::propertyValueTypeError(
                                asyncResp->res,
                                ipv4Static.dump(
                                    2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                                "IPv4StaticAddresses");
                            return;
                        }

                        // One and only one hypervisor instance supported
                        if (ipv4Static.size() != 1)
                        {
                            messages::propertyValueFormatError(
                                asyncResp->res,
                                ipv4Static.dump(
                                    2, ' ', true,
                                    nlohmann::json::error_handler_t::replace),
                                "IPv4StaticAddresses");
                            return;
                        }

                        const nlohmann::json& ipv4Json = ipv4Static[0];
                        // Check if the param is 'null'. If its null, it means
                        // that user wants to delete the IP address. Deleting
                        // the IP address is allowed only if its statically
                        // configured. Deleting the address originated from DHCP
                        // is not allowed.
                        if ((ipv4Json.is_null()) &&
                            (translateDHCPEnabledToBool(ethData.DHCPEnabled,
                                                        true)))
                        {
                            BMCWEB_LOG_INFO
                                << "Ignoring the delete on ipv4StaticAddresses "
                                   "as the interface is DHCP enabled";
                        }
                        else
                        {
                            handleHypervisorIPv4StaticPatch(ifaceId, ipv4Static,
                                                            asyncResp);
                        }
                    }

                    if (hostName)
                    {
                        handleHostnamePatch(*hostName, asyncResp);
                    }

                    if (dhcpv4)
                    {
                        setDHCPEnabled(ifaceId, *ipv4DHCPEnabled, asyncResp);
                    }

                    // Set this interface to disabled/inactive. This will be set
                    // to enabled/active by the pldm once the hypervisor
                    // consumes the updated settings from the user.
                    setIPv4InterfaceEnabled(ifaceId, false, asyncResp);
                });
            asyncResp->res.result(boost::beast::http::status::accepted);
        });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/hypervisor/ResetActionInfo/")
        .privileges(redfish::privileges::getActionInfo)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                // Only return action info if hypervisor D-Bus object present
                crow::connections::systemBus->async_method_call(
                    [asyncResp](
                        const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string, std::vector<std::string>>>& objInfo) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error " << ec;

                            // No hypervisor objects found by mapper
                            if (ec.value() == boost::system::errc::io_error)
                            {
                                messages::resourceNotFound(asyncResp->res,
                                                           "hypervisor",
                                                           "ResetActionInfo");
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // One and only one hypervisor instance supported
                        if (objInfo.size() != 1)
                        {
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // The hypervisor object only support the ability to
                        // turn On The system object Action should be utilized
                        // for other operations
                        asyncResp->res.jsonValue = {
                            {"@odata.type", "#ActionInfo.v1_1_2.ActionInfo"},
                            {"@odata.id",
                             "/redfish/v1/Systems/hypervisor/ResetActionInfo"},
                            {"Name", "Reset Action Info"},
                            {"Id", "ResetActionInfo"},
                            {"Parameters",
                             {{{"Name", "ResetType"},
                               {"Required", true},
                               {"DataType", "String"},
                               {"AllowableValues", {"On"}}}}}};
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/state/hypervisor0",
                    std::array<const char*, 1>{
                        "xyz.openbmc_project.State.Host"});
            });

    BMCWEB_ROUTE(app,
                 "/redfish/v1/Systems/hypervisor/Actions/ComputerSystem.Reset/")
        .privileges(redfish::privileges::postComputerSystem)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                std::optional<std::string> resetType;
                if (!json_util::readJson(req, asyncResp->res, "ResetType",
                                         resetType))
                {
                    // readJson adds appropriate error to response
                    return;
                }

                if (!resetType)
                {
                    messages::actionParameterMissing(
                        asyncResp->res, "ComputerSystem.Reset", "ResetType");
                    return;
                }

                // Hypervisor object only support On operation
                if (resetType != "On")
                {
                    messages::propertyValueNotInList(asyncResp->res, *resetType,
                                                     "ResetType");
                    return;
                }

                std::string command =
                    "xyz.openbmc_project.State.Host.Transition.On";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, resetType](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "D-Bus responses error: " << ec;
                            if (ec.value() ==
                                boost::asio::error::invalid_argument)
                            {
                                messages::actionParameterNotSupported(
                                    asyncResp->res, *resetType, "Reset");
                                return;
                            }

                            if (ec.value() ==
                                boost::asio::error::host_unreachable)
                            {
                                messages::resourceNotFound(asyncResp->res,
                                                           "Actions", "Reset");
                                return;
                            }

                            messages::internalError(asyncResp->res);
                            return;
                        }
                        messages::success(asyncResp->res);
                    },
                    "xyz.openbmc_project.State.Hypervisor",
                    "/xyz/openbmc_project/state/hypervisor0",
                    "org.freedesktop.DBus.Properties", "Set",
                    "xyz.openbmc_project.State.Host", "RequestedHostTransition",
                    std::variant<std::string>{std::move(command)});
            });
}

}