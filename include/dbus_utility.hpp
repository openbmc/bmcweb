// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "boost_formatters.hpp"
#include "dbus_singleton.hpp"
#include "logging.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <regex>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace dbus
{

namespace utility
{

// clang-format off
using DbusVariantType = std::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>,
    std::vector<std::string>,
    std::vector<double>,
    std::string,
    int64_t,
    uint64_t,
    double,
    int32_t,
    uint32_t,
    int16_t,
    uint16_t,
    uint8_t,
    bool,
    std::vector<uint32_t>,
    std::vector<uint16_t>,
    sdbusplus::message::object_path,
    std::tuple<uint64_t, std::vector<std::tuple<std::string, double, uint64_t>>>,
    std::vector<sdbusplus::message::object_path>,
    std::vector<std::tuple<std::string, std::string>>,
    std::vector<std::tuple<uint32_t, std::vector<uint32_t>>>,
    std::vector<std::tuple<uint32_t, size_t>>,
    std::vector<std::tuple<
      std::vector<std::tuple<sdbusplus::message::object_path, std::string>>,
      std::string, std::string, uint64_t>>,
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>,
    std::vector<std::tuple<std::string, uint64_t, std::string, double>>,
    std::vector<std::tuple<std::string, std::string, uint64_t, std::string>>
 >;

// clang-format on
using DBusPropertiesMap = std::vector<std::pair<std::string, DbusVariantType>>;
using DBusInterfacesMap =
    std::vector<std::pair<std::string, DBusPropertiesMap>>;
using ManagedObjectType =
    std::vector<std::pair<sdbusplus::message::object_path, DBusInterfacesMap>>;

// Map of service name to list of interfaces
using MapperServiceMap =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

// Map of object paths to MapperServiceMaps
using MapperGetSubTreeResponse =
    std::vector<std::pair<std::string, MapperServiceMap>>;

using MapperGetObject =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

using MapperGetAncestorsResponse = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using MapperGetSubTreePathsResponse = std::vector<std::string>;

using MapperEndPoints = std::vector<std::string>;

inline void escapePathForDbus(std::string& path)
{
    const static std::regex reg("[^A-Za-z0-9_/]");
    std::regex_replace(path.begin(), path.begin(), path.end(), reg, "_");
}

inline void logError(const boost::system::error_code& ec)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBus error: {}, cannot call method", ec);
    }
}

// gets the string N strings deep into a path
// i.e.  /0th/1st/2nd/3rd
inline bool getNthStringFromPath(const std::string& path, int index,
                                 std::string& result)
{
    if (index < 0)
    {
        return false;
    }

    std::filesystem::path p1(path);
    int count = -1;
    for (const auto& element : p1)
    {
        if (element.has_filename())
        {
            ++count;
            if (count == index)
            {
                result = element.stem().string();
                break;
            }
        }
    }
    return count >= index;
}

inline void
    getAllProperties(const std::string& service, const std::string& objectPath,
                     const std::string& interface,
                     std::function<void(const boost::system::error_code&,
                                        const DBusPropertiesMap&)>&& callback)
{
    sdbusplus::asio::getAllProperties(*crow::connections::systemBus, service,
                                      objectPath, interface,
                                      std::move(callback));
}

template <typename PropertyType>
inline void getProperty(
    const std::string& service, const std::string& objectPath,
    const std::string& interface, const std::string& propertyName,
    std::function<void(const boost::system::error_code&, const PropertyType&)>&&
        callback)
{
    sdbusplus::asio::getProperty<PropertyType>(
        *crow::connections::systemBus, service, objectPath, interface,
        propertyName, std::move(callback));
}

template <typename PropertyType>
inline void getProperty(
    sdbusplus::asio::connection& /*conn*/, const std::string& service,
    const std::string& objectPath, const std::string& interface,
    const std::string& propertyName,
    std::function<void(const boost::system::error_code&, const PropertyType&)>&&
        callback)
{
    getProperty(service, objectPath, interface, propertyName,
                std::move(callback));
}

inline void getAllProperties(
    sdbusplus::asio::connection& /*conn*/, const std::string& service,
    const std::string& objectPath, const std::string& interface,
    std::function<void(const boost::system::error_code&,
                       const DBusPropertiesMap&)>&& callback)
{
    getAllProperties(service, objectPath, interface, std::move(callback));
}

inline void checkDbusPathExists(const std::string& path,
                                std::function<void(bool)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback = std::move(callback)](const boost::system::error_code& ec,
                                         const MapperGetObject& objectNames) {
            callback(!ec && !objectNames.empty());
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path,
        std::array<std::string, 0>());
}

inline void
    getSubTree(const std::string& path, int32_t depth,
               std::span<const std::string_view> interfaces,
               std::function<void(const boost::system::error_code&,
                                  const MapperGetSubTreeResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", path, depth,
        interfaces);
}

inline void getSubTreePaths(
    const std::string& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
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

inline void getAssociatedSubTree(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTree",
        associatedPath, path, depth, interfaces);
}

inline void getAssociatedSubTreePaths(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
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

inline void getAssociatedSubTreeById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](
            const boost::system::error_code& ec,
            const MapperGetSubTreeResponse& subtree) { callback(ec, subtree); },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetAssociatedSubTreeById", id,
        path, subtreeInterfaces, association, endpointInterfaces);
}

inline void getAssociatedSubTreePathsById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
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

inline void getDbusObject(
    const std::string& path, std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetObject&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code& ec,
                                        const MapperGetObject& object) {
            callback(ec, object);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", path, interfaces);
}

inline void getAssociationEndPoints(
    const std::string& path,
    std::function<void(const boost::system::error_code&,
                       const MapperEndPoints&)>&& callback)
{
    getProperty<MapperEndPoints>("xyz.openbmc_project.ObjectMapper", path,
                                 "xyz.openbmc_project.Association", "endpoints",
                                 std::move(callback));
}

inline void getManagedObjects(
    const std::string& service, const sdbusplus::message::object_path& path,
    std::function<void(const boost::system::error_code&,
                       const ManagedObjectType&)>&& callback)
{
    crow::connections::systemBus->async_method_call(
        [callback{std::move(callback)}](const boost::system::error_code& ec,
                                        const ManagedObjectType& objects) {
            callback(ec, objects);
        },
        service, path, "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

} // namespace utility
} // namespace dbus
