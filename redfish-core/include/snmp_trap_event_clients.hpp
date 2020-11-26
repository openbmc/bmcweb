namespace redfish
{

inline void
    doGetSnmpTrapClientdata(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& objectPath)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, "xyz.openbmc_project.Network.SNMP",
        objectPath, "xyz.openbmc_project.Network.Client",
        [asyncResp](const boost::system::error_code ec,
                    const dbus::utility::DBusPropertiesMap& propertiesList) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetSubTree " << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string* address = nullptr;
        const uint16_t* port = nullptr;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), propertiesList, "Address",
            address, "Port", port);

        if (!success)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        if (address != nullptr && port != nullptr)
        {
            std::string destination = "snmp://";
            destination.append(*address);
            destination.append(":");
            destination.append(std::to_string(*port));

            asyncResp->res.jsonValue["Destination"] = std::move(destination);
        }
        });
}

inline void
    getSnmpTrapClientdata(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& id, const std::string& objectPath)
{
    asyncResp->res.jsonValue["@odata.type"] =
        "#EventDestination.v1_8_0.EventDestination";
    asyncResp->res.jsonValue["Protocol"] = "SNMPv2c";
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "EventService", "Subscriptions", id);

    asyncResp->res.jsonValue["Id"] = id;
    asyncResp->res.jsonValue["Name"] = "Event Destination " + id;

    asyncResp->res.jsonValue["SubscriptionType"] = "SNMPTrap";
    asyncResp->res.jsonValue["EventFormatType"] = "Event";

    std::shared_ptr<Subscription> subValue =
        EventServiceManager::getInstance().getSubscription(id);
    if (subValue != nullptr)
    {
        asyncResp->res.jsonValue["Context"] = subValue->customText;
    }
    else
    {
        asyncResp->res.jsonValue["Context"] = "";
    }

    doGetSnmpTrapClientdata(asyncResp, objectPath);
}

inline void
    getSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& id)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, id](const boost::system::error_code ec,
                        dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetManagedObjects "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        for (const auto& objpath : resp)
        {
            sdbusplus::message::object_path path(objpath.first);
            const std::string snmpId = path.filename();
            if (snmpId.empty())
            {
                BMCWEB_LOG_ERROR << "The SNMP client ID is wrong";
                messages::internalError(asyncResp->res);
                return;
            }
            const std::string subscriptionId = "snmp" + snmpId;
            if (id != subscriptionId)
            {
                continue;
            }

            getSnmpTrapClientdata(asyncResp, id, objpath.first);
            return;
        }

        messages::resourceNotFound(asyncResp->res, "Subscriptions", id);
        EventServiceManager::getInstance().deleteSubscription(id);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    createSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& host, const uint16_t& snmpTrapPort,
                         const std::shared_ptr<Subscription>& subValue)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, subValue](const boost::system::error_code ec,
                              const std::string& dbusSNMPid) {
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        sdbusplus::message::object_path path(dbusSNMPid);
        const std::string snmpId = path.filename();
        if (snmpId.empty())
        {
            messages::internalError(asyncResp->res);
            return;
        }

        std::string subscriptionId = "snmp" + snmpId;

        EventServiceManager::getInstance().addSubscription(subValue,
                                                           subscriptionId);

        asyncResp->res.addHeader("Location",
                                 "/redfish/v1/EventService/Subscriptions/" +
                                     subscriptionId);
        messages::created(asyncResp->res);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "xyz.openbmc_project.Network.Client.Create", "Client", host,
        snmpTrapPort);
}

inline bool clientAlreadyExists(dbus::utility::ManagedObjectType& resp,
                                const std::string& host,
                                const uint16_t& snmpTrapPort)
{
    for (const auto& object : resp)
    {
        for (const auto& interface : object.second)
        {
            if (interface.first == "xyz.openbmc_project.Network.Client")
            {
                std::string address;
                uint16_t portNum = 0;
                for (const auto& property : interface.second)
                {
                    if (property.first == "Address")
                    {
                        const std::string* value =
                            std::get_if<std::string>(&property.second);
                        if (value == nullptr)
                        {
                            continue;
                        }
                        address = *value;
                    }
                    else if (property.first == "Port")
                    {
                        const uint16_t* value =
                            std::get_if<uint16_t>(&property.second);
                        if (value == nullptr)
                        {
                            continue;
                        }
                        portNum = *value;
                    }
                }

                if (address == host && portNum == snmpTrapPort)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

inline void
    addSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& host, const uint16_t& snmpTrapPort,
                      const std::string& destUrl,
                      const std::shared_ptr<Subscription>& subValue)
{
    // Check whether the client already exists
    crow::connections::systemBus->async_method_call(
        [asyncResp, host, snmpTrapPort, destUrl,
         subValue](const boost::system::error_code ec,
                   dbus::utility::ManagedObjectType& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "D-Bus response error on GetManagedObjects "
                             << ec;
            messages::internalError(asyncResp->res);
            return;
        }

        if (clientAlreadyExists(resp, host, snmpTrapPort))
        {
            messages::resourceAlreadyExists(
                asyncResp->res, "EventDestination.v1_8_0.EventDestination",
                "Destination", destUrl);
            return;
        }

        // Create the snmp client
        createSnmpTrapClient(asyncResp, host, snmpTrapPort, subValue);
        },
        "xyz.openbmc_project.Network.SNMP",
        "/xyz/openbmc_project/network/snmp/manager",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void
    getSnmpSubscriptionList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& snmpId,
                            nlohmann::json& memberArray)
{
    const std::string subscriptionId = "snmp" + snmpId;

    nlohmann::json::object_t member;
    member["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "EventService", "Subscriptions", subscriptionId);
    memberArray.push_back(std::move(member));

    asyncResp->res.jsonValue["Members@odata.count"] = memberArray.size();
}

inline void
    deleteSnmpTrapClient(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& param)
{
    std::string_view snmpTrapId = param;

    // Erase "snmp" in the request to find the corresponding
    // dbus snmp client id. For example, the snmpid in the
    // request is "snmp1", which will be "1" after being erased.
    snmpTrapId.remove_prefix(4);

    const std::string snmpPath =
        "/xyz/openbmc_project/network/snmp/manager/" + std::string(snmpTrapId);

    crow::connections::systemBus->async_method_call(
        [asyncResp, param](const boost::system::error_code ec) {
        if (ec)
        {
            // The snmp trap id is incorrect
            if (ec.value() == EBADR)
            {
                messages::resourceNotFound(asyncResp->res, "Subscription",
                                           param);
                return;
            }
            messages::internalError(asyncResp->res);
            return;
        }
        messages::success(asyncResp->res);
        },
        "xyz.openbmc_project.Network.SNMP", snmpPath,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

} // namespace redfish