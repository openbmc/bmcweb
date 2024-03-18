// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "boost_formatters.hpp"
#include "dbus_singleton.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
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

// List of interfaces
using InterfaceList = std::vector<std::string>;

// Map of service name to list of interfaces
using MapperServiceMap = std::vector<std::pair<std::string, InterfaceList>>;

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

void escapePathForDbus(std::string& path);

void logError(const boost::system::error_code& ec);

// gets the string N strings deep into a path
// i.e.  /0th/1st/2nd/3rd
bool getNthStringFromPath(const std::string& path, int index,
                          std::string& result);

void getAllProperties(const std::string& service, const std::string& objectPath,
                      const std::string& interface,
                      std::function<void(const boost::system::error_code&,
                                         const DBusPropertiesMap&)>&& callback);

template <typename PropertyType>
void getProperty(const std::string& service, const std::string& objectPath,
                 const std::string& interface, const std::string& propertyName,
                 std::function<void(const boost::system::error_code&,
                                    const PropertyType&)>&& callback)
{
    sdbusplus::asio::getProperty<PropertyType>(
        *crow::connections::systemBus, service, objectPath, interface,
        propertyName, std::move(callback));
}

template <typename PropertyType>
void getProperty(sdbusplus::asio::connection& /*conn*/,
                 const std::string& service, const std::string& objectPath,
                 const std::string& interface, const std::string& propertyName,
                 std::function<void(const boost::system::error_code&,
                                    const PropertyType&)>&& callback)
{
    getProperty(service, objectPath, interface, propertyName,
                std::move(callback));
}

void getAllProperties(sdbusplus::asio::connection& /*conn*/,
                      const std::string& service, const std::string& objectPath,
                      const std::string& interface,
                      std::function<void(const boost::system::error_code&,
                                         const DBusPropertiesMap&)>&& callback);

void checkDbusPathExists(const std::string& path,
                         std::function<void(bool)>&& callback);

void getSubTree(
    const std::string& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback);

void getSubTreePaths(
    const std::string& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback);

void getAssociatedSubTree(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback);

void getAssociatedSubTreePaths(
    const sdbusplus::message::object_path& associatedPath,
    const sdbusplus::message::object_path& path, int32_t depth,
    std::span<const std::string_view> interfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback);

void getAssociatedSubTreeById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreeResponse&)>&& callback);

void getAssociatedSubTreePathsById(
    const std::string& id, const std::string& path,
    std::span<const std::string_view> subtreeInterfaces,
    std::string_view association,
    std::span<const std::string_view> endpointInterfaces,
    std::function<void(const boost::system::error_code&,
                       const MapperGetSubTreePathsResponse&)>&& callback);

void getDbusObject(const std::string& path,
                   std::span<const std::string_view> interfaces,
                   std::function<void(const boost::system::error_code&,
                                      const MapperGetObject&)>&& callback);

void getAssociationEndPoints(
    const std::string& path,
    std::function<void(const boost::system::error_code&,
                       const MapperEndPoints&)>&& callback);

void getManagedObjects(
    const std::string& service, const sdbusplus::message::object_path& path,
    std::function<void(const boost::system::error_code&,
                       const ManagedObjectType&)>&& callback);
} // namespace utility
} // namespace dbus
