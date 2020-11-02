#pragma once
#include "logging.h"

#include <app.h>

#include <boost/container/flat_map.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message/types.hpp>

#include <map>
#include <string>
#include <vector>

namespace redfish
{

using GetManagedPropertyType = boost::container::flat_map<
    std::string, std::variant<std::string, bool, uint8_t, int16_t, uint16_t,
                              int32_t, uint32_t, int64_t, uint64_t, double>>;

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<std::string, GetManagedPropertyType>>;

class DbusInterface
{

  private:
   // multimap is required as we need to create a match for 
   // interface added and interface removed on the same object.

    std::multimap<std::string, std::unique_ptr<sdbusplus::bus::match_t>>
        objectMap;

  public:
    template <typename MessageHandler, typename PropertyHandler,
              typename... InputArgs>
    void getDbusObject(MessageHandler&& handler, PropertyHandler&& propHandler,
                       const std::string& service, const std::string& objpath,
                       const std::string& interface, const std::string& method,
                       const InputArgs&... a)
    {
        if (interface == "xyz.openbmc_project.ObjectMapper" &&
            method == "GetObject")
        {
            crow::connections::systemBus->async_method_call(
                [this, objpath, handler,
                 propHandler](const boost::system::error_code ec,
                              const ManagedObjectType& ldapObjects) {
                    if (!ec)
                    {
                        std::string propMatchString =
                            ("type='signal',"
                             "interface='org.freedesktop.DBus.Properties',"
                             "path_namespace='" +
                             objpath +
                             "',"
                             "member='PropertiesChanged'");
                        objectMap.emplace(
                            objpath,
                            std::make_unique<sdbusplus::bus::match::match>(
                                *crow::connections::systemBus, propMatchString,
                                propHandler, nullptr));
                        BMCWEB_LOG_DEBUG
                            << "********Printing Monitor Map**************";
                        for (auto object : objectMap)
                        {
                            BMCWEB_LOG_DEBUG << object.first;
                        }
                        BMCWEB_LOG_DEBUG
                            << "********Printing done**************";

                        handler(ec, ldapObjects);
                    }
                },
                service, objpath, interface, method, a...);
        }
    }

    template <typename MessageHandler, typename ObjectHandler,
              typename... InputArgs>
    void getAllDbusObjects(MessageHandler&& handler,
                           ObjectHandler&& notifyObjHandler,
                           const std::string& service,
                           const std::string& objpath,
                           const std::string& interface,
                           const std::string& method, const InputArgs&... a)
    {
        if (interface == "org.freedesktop.DBus.ObjectManager" &&
            method == "GetManagedObjects")
        {
            crow::connections::systemBus->async_method_call(
                [this, objpath, handler,
                 notifyObjHandler](const boost::system::error_code ec,
                                   const GetManagedObjectsType& objects) {
                    // Add the interface added/removed match for the objects
                    // under this.
                    if (!ec)
                    {
                        std::string objMatchString =
                            ("type='signal',"
                             "interface='org.freedesktop.DBus.ObjectManager',"
                             "path_namespace='" +
                             objpath +
                             "',"
                             "member='InterfacesAdded'");
                        BMCWEB_LOG_DEBUG << "Creating match " << objMatchString;
                        objectMap.emplace(std::make_pair(
                            objpath,
                            std::make_unique<sdbusplus::bus::match::match>(
                                *crow::connections::systemBus, objMatchString,
                                notifyObjHandler, nullptr)));
                        objMatchString =
                            ("type='signal',"
                             "interface='org.freedesktop.DBus.ObjectManager',"
                             "path_namespace='" +
                             objpath +
                             "',"
                             "member='InterfacesRemoved'");
                        BMCWEB_LOG_DEBUG << "Creating match " << objMatchString;
                        objectMap.emplace(std::make_pair(
                            objpath,
                            std::make_unique<sdbusplus::bus::match::match>(
                                *crow::connections::systemBus, objMatchString,
                                notifyObjHandler, nullptr)));
                        // Add the property handler match for  all the objects.
                        for (auto& object : objects)
                        {
                            std::string propMatchString =
                                ("type='signal',"
                                 "interface='org.freedesktop.DBus.Properties',"
                                 "path_namespace='" +
                                 object.first.str +
                                 "',"
                                 "member='PropertiesChanged'");
                            objectMap.emplace(
                                object.first,
                                std::make_unique<sdbusplus::bus::match::match>(
                                    *crow::connections::systemBus,
                                    propMatchString, notifyObjHandler,
                                    nullptr));
                            BMCWEB_LOG_DEBUG << "Creating match "
                                             << propMatchString;
                        }
                        BMCWEB_LOG_DEBUG
                            << "********Printing Monitor Map**************";
                        for (auto& object : objectMap)
                        {
                            BMCWEB_LOG_DEBUG << object.first;
                        }
                        BMCWEB_LOG_DEBUG
                            << "********Printing done**************";

                        handler(ec, objects);
                    }
                },
                service, objpath, interface, method, a...);
        }
    }
           

};

} // namespace redfish
