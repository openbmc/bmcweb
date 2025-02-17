// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "boost_formatters.hpp"
#include "dbus_singleton.hpp"
#include "logging.hpp"

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

void escapePathForDbus(std::string& path);

void logError(const boost::system::error_code& ec);

void getAllProperties(const std::string& service, const std::string& objectPath,
                      const std::string& interface,
                      std::function<void(const boost::system::error_code&,
                                         const DBusPropertiesMap&)>&& callback);

template <std::size_t N, typename FirstArg, typename... Rest>
struct strip_first_n_args;

template <std::size_t N, typename FirstArg, typename... Rest>
struct strip_first_n_args<N, std::tuple<FirstArg, Rest...>> :
    strip_first_n_args<N - 1, std::tuple<Rest...>>
{};

template <typename FirstArg, typename... Rest>
struct strip_first_n_args<0, std::tuple<FirstArg, Rest...>>
{
    using type = std::tuple<FirstArg, Rest...>;
};
template <std::size_t N>
struct strip_first_n_args<N, std::tuple<>>
{
    using type = std::tuple<>;
};

template <std::size_t N, typename... Args>
using strip_first_n_args_t = typename strip_first_n_args<N, Args...>::type;

// Small helper class for stripping off the error code from the function
// argument definitions so unpack can be called appropriately
template <typename T>
using strip_first_arg = strip_first_n_args<1, T>;

template <typename T>
using strip_first_arg_t = typename strip_first_arg<T>::type;

template <typename MessageHandler>
void handle_async_method_call_response(MessageHandler&& handler,
                                       const boost::system::error_code& ec,
                                       sdbusplus::message::message& msg)
{
    using ResultType =
        strip_first_arg_t<boost::callable_traits::args_t<MessageHandler>>;
    using FunctionTupleType = sdbusplus::utility::decay_tuple_t<ResultType>;
    FunctionTupleType responseData;
    if (!ec)
    {
        try
        {
            sdbusplus::utility::read_into_tuple(responseData, msg);
        }
        catch (const std::exception&)
        {
            // Set error code if not already set
            boost::system::error_code ec2 =
                boost::system::errc::make_error_code(
                    boost::system::errc::invalid_argument);

            auto response =
                std::tuple_cat(std::make_tuple(ec2), std::move(responseData));
            std::apply(handler, response);
            return;
        }
    }
    auto response =
        std::tuple_cat(std::make_tuple(ec), std::move(responseData));
    std::apply(handler, response);
}

template <typename MessageHandler, typename... InputArgs>
void async_method_call(MessageHandler&& handler, const std::string& service,
                       const std::string& objpath, const std::string& interf,
                       const std::string& method, const InputArgs&... a)
{
    static uint64_t callId = 0;
    callId++;

    BMCWEB_LOG_DEBUG("{} Method Call {} {} {} {}", callId, service, objpath,
                     interf, method);
    auto handler2 = [handler = std::move(handler)](
                        const boost::system::error_code& ec,
                        sdbusplus::message::message& msg) mutable {
        handle_async_method_call_response<MessageHandler>(
            std::forward<MessageHandler>(handler), ec, msg);
    };

    crow::connections::systemBus->async_method_call(
        std::move(handler2), service, objpath, interf, method, a...);
}

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
