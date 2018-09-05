#pragma once
#include <crow/app.h>
#include <crow/websocket.h>

#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <sdbusplus/bus/match.hpp>

namespace nlohmann
{
template <typename... Args>
struct adl_serializer<sdbusplus::message::variant<Args...>>
{
    static void to_json(json& j, const sdbusplus::message::variant<Args...>& v)
    {
        mapbox::util::apply_visitor([&](auto&& val) { j = val; }, v);
    }
};
} // namespace nlohmann

namespace crow
{
namespace dbus_monitor
{

struct DbusWebsocketSession
{
    std::vector<std::unique_ptr<sdbusplus::bus::match::match>> matches;
    boost::container::flat_set<std::string> interfaces;
};

static boost::container::flat_map<crow::websocket::Connection*,
                                  DbusWebsocketSession>
    sessions;

inline int onPropertyUpdate(sd_bus_message* m, void* userdata,
                            sd_bus_error* ret_error)
{
    if (ret_error == nullptr || sd_bus_error_is_set(ret_error))
    {
        BMCWEB_LOG_ERROR << "Got sdbus error on match";
        return 0;
    }
    crow::websocket::Connection* connection =
        static_cast<crow::websocket::Connection*>(userdata);
    auto thisSession = sessions.find(connection);
    if (thisSession == sessions.end())
    {
        BMCWEB_LOG_ERROR << "Couldn't find dbus connection " << connection;
        return 0;
    }
    sdbusplus::message::message message(m);
    using VariantType = sdbusplus::message::variant<std::string, bool, int64_t,
                                                    uint64_t, double>;
    nlohmann::json j{{"event", message.get_member()},
                     {"path", message.get_path()}};
    if (strcmp(message.get_member(), "PropertiesChanged") == 0)
    {
        std::string interface_name;
        boost::container::flat_map<std::string, VariantType> values;
        message.read(interface_name, values);
        j["properties"] = values;
        j["interface"] = std::move(interface_name);
    }
    else if (strcmp(message.get_member(), "InterfacesAdded") == 0)
    {
        std::string object_name;
        boost::container::flat_map<
            std::string, boost::container::flat_map<std::string, VariantType>>
            values;
        message.read(object_name, values);
        for (const std::pair<
                 std::string,
                 boost::container::flat_map<std::string, VariantType>>& paths :
             values)
        {
            auto it = thisSession->second.interfaces.find(paths.first);
            if (it != thisSession->second.interfaces.end())
            {
                j["interfaces"][paths.first] = paths.second;
            }
        }
    }
    else
    {
        BMCWEB_LOG_CRITICAL << "message " << message.get_member()
                            << " was unexpected";
        return 0;
    }

    connection->sendText(j.dump());
    return 0;
};

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    BMCWEB_ROUTE(app, "/subscribe")
        .websocket()
        .onopen([&](crow::websocket::Connection& conn) {
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " opened";
            sessions[&conn] = DbusWebsocketSession();
        })
        .onclose([&](crow::websocket::Connection& conn,
                     const std::string& reason) { sessions.erase(&conn); })
        .onmessage([&](crow::websocket::Connection& conn,
                       const std::string& data, bool is_binary) {
            DbusWebsocketSession& thisSession = sessions[&conn];
            BMCWEB_LOG_DEBUG << "Connection " << &conn << " recevied " << data;
            nlohmann::json j = nlohmann::json::parse(data, nullptr, false);
            if (j.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Unable to parse json data for monitor";
                conn.close("Unable to parse json request");
                return;
            }
            nlohmann::json::iterator interfaces = j.find("interfaces");
            if (interfaces != j.end())
            {
                thisSession.interfaces.reserve(interfaces->size());
                for (auto& interface : *interfaces)
                {
                    const std::string* str =
                        interface.get_ptr<const std::string*>();
                    if (str != nullptr)
                    {
                        thisSession.interfaces.insert(*str);
                    }
                }
            }

            nlohmann::json::iterator paths = j.find("paths");
            if (paths != j.end())
            {
                int interfaceCount = thisSession.interfaces.size();
                if (interfaceCount == 0)
                {
                    interfaceCount = 1;
                }
                // Reserve our matches upfront.  For each path there is 1 for
                // interfacesAdded, and InterfaceCount number for
                // PropertiesChanged
                thisSession.matches.reserve(thisSession.matches.size() +
                                            paths->size() *
                                                (1 + interfaceCount));
            }
            std::string object_manager_match_string;
            std::string properties_match_string;
            std::string object_manager_interfaces_match_string;
            // These regexes derived on the rules here:
            // https://dbus.freedesktop.org/doc/dbus-specification.html#message-protocol-names
            std::regex validPath("^/([A-Za-z0-9_]+/?)*$");
            std::regex validInterface(
                "^[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)+$");

            for (const auto& thisPath : *paths)
            {
                const std::string* thisPathString =
                    thisPath.get_ptr<const std::string*>();
                if (thisPathString == nullptr)
                {
                    BMCWEB_LOG_ERROR << "subscribe path isn't a string?";
                    conn.close();
                    return;
                }
                if (!std::regex_match(*thisPathString, validPath))
                {
                    BMCWEB_LOG_ERROR << "Invalid path name " << *thisPathString;
                    conn.close();
                    return;
                }
                properties_match_string =
                    ("type='signal',"
                     "interface='org.freedesktop.DBus.Properties',"
                     "path_namespace='" +
                     *thisPathString +
                     "',"
                     "member='PropertiesChanged'");
                // If interfaces weren't specified, add a single match for all
                // interfaces
                if (thisSession.interfaces.size() == 0)
                {
                    BMCWEB_LOG_DEBUG << "Creating match "
                                     << properties_match_string;

                    thisSession.matches.emplace_back(
                        std::make_unique<sdbusplus::bus::match::match>(
                            *crow::connections::systemBus,
                            properties_match_string, onPropertyUpdate, &conn));
                }
                else
                {
                    // If interfaces were specified, add a match for each
                    // interface
                    for (const std::string& interface : thisSession.interfaces)
                    {
                        if (!std::regex_match(interface, validInterface))
                        {
                            BMCWEB_LOG_ERROR << "Invalid interface name "
                                             << interface;
                            conn.close();
                            return;
                        }
                        std::string ifaceMatchString = properties_match_string +
                                                       ",arg0='" + interface +
                                                       "'";
                        BMCWEB_LOG_DEBUG << "Creating match "
                                         << ifaceMatchString;
                        thisSession.matches.emplace_back(
                            std::make_unique<sdbusplus::bus::match::match>(
                                *crow::connections::systemBus, ifaceMatchString,
                                onPropertyUpdate, &conn));
                    }
                }
                object_manager_match_string =
                    ("type='signal',"
                     "interface='org.freedesktop.DBus.ObjectManager',"
                     "path_namespace='" +
                     *thisPathString +
                     "',"
                     "member='InterfacesAdded'");
                BMCWEB_LOG_DEBUG << "Creating match "
                                 << object_manager_match_string;
                thisSession.matches.emplace_back(
                    std::make_unique<sdbusplus::bus::match::match>(
                        *crow::connections::systemBus,
                        object_manager_match_string, onPropertyUpdate, &conn));
            }
        });
}
} // namespace dbus_monitor
} // namespace crow
