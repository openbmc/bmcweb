// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "dbus_utility.hpp"

#include "boost_formatters.hpp"
#include "dbus_singleton.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <regex>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace dbus
{

namespace utility
{

void escapePathForDbus(std::string& path)
{
    const static std::regex reg("[^A-Za-z0-9_/]");
    std::regex_replace(path.begin(), path.begin(), path.end(), reg, "_");
}

void logError(const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error: {}, cannot call method", ec);
    }
}

void getAllProperties(const std::string& service, const std::string& objectPath,
                      const std::string& interface,
                      std::function<void(const boost::system::error_code&,
                                         const DBusPropertiesMap&)>&& callback)
{
    sdbusplus::asio::getAllProperties(*crow::connections::systemBus, service,
                                      objectPath, interface,
                                      std::move(callback));
}

void getAllProperties(sdbusplus::asio::connection& /*conn*/,
                      const std::string& service, const std::string& objectPath,
                      const std::string& interface,
                      std::function<void(const boost::system::error_code&,
                                         const DBusPropertiesMap&)>&& callback)
{
    getAllProperties(service, objectPath, interface, std::move(callback));
}

void checkDbusPathExists(const std::string& path,
                         std::function<void(bool)>&& callback)
{
    dbus::utility::async_method_call(
        [callback = std::move(callback)](const boost::system::error_code& ec,
                                         const MapperGetObject& objectNames) {
            callback(!ec && !objectNames.empty());
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

void getSubTree(const std::string& path, int32_t depth,
                std::span<const std::string_view> interfaces,
                std::function<void(const boost::system::error_code&,
                                   const MapperGetSubTreeResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, depth,
        interfaces);
}

void getSubTreePaths(
    const std::string& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreePathsResponse& subtreePaths) {
            callback(ec, subtreePaths);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", path, depth,
        interfaces);
}

void getAssociatedSubTree(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTree",
        associatedPath, path, depth, interfaces);
}

void getAssociatedSubTreePaths(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreePathsResponse& subtreePaths) {
            callback(ec, subtreePaths);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTreePaths",
        associatedPath, path, depth, interfaces);
}

void getAssociatedSubTreeById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTreeById", id,
        path, subtreeInterfaces, association, endpointInterfaces);
}

void getAssociatedSubTreePathsById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreePathsResponse& subtreePaths) {
            callback(ec, subtreePaths);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTreePathsById", id,
        path, subtreeInterfaces, association, endpointInterfaces);
}

void getDbusObject(const std::string& path,
                   std::span<const std::string_view> interfaces,
                   std::function<void(const boost::system::error_code&,
                                      const MapperGetObject&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code& ec,
                                        const MapperGetObject& object) {
            callback(ec, object);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path, interfaces);
}

void getAssociationEndPoints(
    const std::string& path,
    std::function<void(const boost::system::error_code&,
                       const MapperEndPoints&)>&& callback)
{
    getProperty<MapperEndPoints>("xyz.openbmc_project.ObjectMapper", path,
                                 "xyz.openbmc_project.Association", "endpoints",
                                 std::move(callback));
}

void getManagedObjects(const std::string& service,
                       const sdbusplus::message::object_path& path,
                       std::function<void(const boost::system::error_code&,
                                          const ManagedObjectType&)>&& callback)
{
    dbus::utility::async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code& ec,
                                        const ManagedObjectType& objects) {
            callback(ec, objects);
        },
        service, path, "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

} // namespace utility
} // namespace dbus
