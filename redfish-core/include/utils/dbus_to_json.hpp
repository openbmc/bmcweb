
#pragma once
#include "dbus_utility.hpp"

#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <ranges>
#include <string_view>
#include <variant>
#include <vector>

namespace DU = dbus::utility;

namespace redfish
{
template <typename Handler, typename Val>
concept InvokableWithObjectPath = requires(
    Handler h, const Val& v, const sdbusplus::message::object_path& p) {
                                      {
                                          h(p, v)
                                      } -> std::convertible_to<nlohmann::json>;
                                  };
template <typename Handler, typename Val>
concept InvokableWithValue = requires(Handler h, const Val& v) {
                                 {
                                     h(v)
                                 } -> std::convertible_to<nlohmann::json>;
                             };
template <typename Handler, typename Val>
concept InvokableWithIfaceName =
    requires(Handler h, const Val& v, std::string_view vw) {
        {
            h(vw, v)
        } -> std::convertible_to<nlohmann::json>;
    };

template <typename Handler, typename Val>
concept InvokableWithPathAndIfaceName =
    requires(Handler h, const Val& v, const sdbusplus::message::object_path& p,
             std::string_view vw) {
        {
            h(p, vw, v)
        } -> std::convertible_to<nlohmann::json>;
    };

using AssociationsValType =
    std::vector<std::tuple<std::string, std::string, std::string>>;
struct BaseNodeMapper
{
    virtual std::optional<nlohmann::json>
        visit(const DU::DbusVariantType& var,
              const sdbusplus::message::object_path&,
              std::string_view ifaceName, std::string_view nm) = 0;
    virtual ~BaseNodeMapper() {}
};

template <typename Type>
using MapperHandler = std::function<nlohmann::json(
    const sdbusplus::message::object_path&, std::string_view, std::string_view,
    const Type&)>;
inline auto stringSplitter()
{
    return std::views::split('/') | std::views::transform([](auto&& sub) {
               return std::string(sub.begin(), sub.end());
           });
}
inline auto makeJson(const auto& key, const auto& val)
{
    auto keys = key | stringSplitter();
    std::vector v(keys.begin(), keys.end());
    auto rv = v | std::views::reverse;
    nlohmann::json init;
    init[rv.front()] = val;
    auto newJson = std::accumulate(rv.begin() + 1, rv.end(), init,
                                   [](auto sofar, auto cuurentKey) {
        nlohmann::json j;
        j[cuurentKey] = sofar;
        return j;
    });
    return newJson;
}
struct GenericNodeMapper : BaseNodeMapper
{
    const std::string key;
    GenericNodeMapper(const std::string& k) : key(k) {}
    std::optional<nlohmann::json> visit(const DU::DbusVariantType& var,
                                        const sdbusplus::message::object_path&,
                                        std::string_view,
                                        std::string_view) override
    {
        return std::visit(
            [&](auto&& val) -> std::
                                optional<nlohmann::json> {
            using T = std::decay_t<decltype(val)>;
            if constexpr (!std::is_same_v<T, sdbusplus::message::unix_fd> &&
                          !std::is_same_v<T, sdbusplus::message::object_path> &&
                          !std::is_same_v<
                              T, std::vector<std::tuple<
                                     sdbusplus::message::object_path,
                                     std::string, std::string, std::string>>>)
            {
                return makeJson(key, std::forward<decltype(val)>(val));
            }

            return std::nullopt;
            },
            var);
    }
};
template <typename Type>
struct NodeMapper : BaseNodeMapper
{
    using HandlerType = MapperHandler<Type>;
    HandlerType handler;
    NodeMapper(HandlerType h) : handler(std::move(h)) {}
    std::optional<nlohmann::json>
        visit(const DU::DbusVariantType& var,
              const sdbusplus::message::object_path& path,
              std::string_view ifaceName, std::string_view name) override
    {
        return std::visit(
            [&](auto&& val) -> std::optional<nlohmann::json> {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, Type>)
                {
                    return handler(path, ifaceName, name, val);
                }
                return std::nullopt;
            },
            var);
    }
};
template <typename Type>
inline auto makeNodeMapper(MapperHandler<Type> handler)
{
    return std::make_unique<NodeMapper<Type>>(std::move(handler));
}

inline auto mapToKey(const std::string& key)
{
    return std::make_unique<GenericNodeMapper>(key);
}
template <typename Type>
inline auto mapToKey(const std::string& key)
{
    return makeNodeMapper<Type>(
        [key](const sdbusplus::message::object_path&, std::string_view,
              std::string_view, const Type& val) {
        return makeJson(key, val);
    });
}

template <typename Type, InvokableWithValue<Type> Handler>
inline auto mapToHandler(Handler handler)
{
    return makeNodeMapper<Type>([handler = std::move(handler)](
                                    const sdbusplus::message::object_path&,
                                    std::string_view, std::string_view,
                                    const Type& val) { return handler(val); });
}
template <typename Type, InvokableWithObjectPath<Type> Handler>
inline auto mapToHandler(Handler handler)
{
    return makeNodeMapper<Type>(
        [handler = std::move(handler)](
            const sdbusplus::message::object_path& path, std::string_view,
            std::string_view, const Type& val) { return handler(path, val); });
}

template <typename Type, InvokableWithIfaceName<Type> Handler>
inline auto mapToHandler(Handler handler)
{
    return makeNodeMapper<Type>(
        [handler = std::move(handler)](const sdbusplus::message::object_path&,
                                       std::string_view ifaceName,
                                       std::string_view, const Type& val) {
        return handler(ifaceName, val);
    });
}
template <typename Type, InvokableWithPathAndIfaceName<Type> Handler>
inline auto mapToHandler(Handler handler)
{
    return makeNodeMapper<Type>(
        [handler = std::move(handler)](
            const sdbusplus::message::object_path& path,
            std::string_view ifaceName, std::string_view, const Type& val) {
        return handler(path, ifaceName, val);
    });
}

using handler_pair =
    std::pair<std::string_view, std::unique_ptr<BaseNodeMapper>>;

using ValueExtractor = std::function<nlohmann::json(
    const sdbusplus::message::object_path&, std::string_view,
    const DU::DBusPropertiesMap&, bool)>;

std::optional<nlohmann::json>
    visitIfPresent(const auto& path, const auto& ifaceName, const auto& propMap,
                   const auto& mapper, const auto& name)
{
    auto iter = std::ranges::find_if(propMap,
                                     [&](auto& e) { return e.first == name; });
    if (iter != propMap.end())
    {
        return mapper->visit(iter->second, path, ifaceName, name);
    }
    return std::nullopt;
}
inline auto extractFromInterfaceKeys(const auto& path, const auto& ifaceName,
                                     const DU::DBusPropertiesMap& propMap,
                                     const std::vector<handler_pair>& KeyNames,
                                     bool ignore_unknown)
{
    auto json_nodes =
        KeyNames |
        std::views::transform([&propMap, &path, &ifaceName](
                                  auto& v) -> std::optional<nlohmann::json> {
            auto& [name, mapper] = v;
            return visitIfPresent(path, ifaceName, propMap, mapper, name);
        }) |
        std::views::filter([ignore_unknown](const auto& v) {
            return ignore_unknown ? v.has_value() : true;
        });
    nlohmann::json result;
    for (auto j : json_nodes)
    {
        result.merge_patch(j.value());
    }
    return result;
}
inline auto makeExtractor(std::vector<handler_pair>& keys)
{
    return [&keys](const auto& path, const auto& ifaceName,
                   const DU::DBusPropertiesMap& propMap, bool ignore_unknown) {
        return extractFromInterfaceKeys(path, ifaceName, propMap, keys,
                                        ignore_unknown);
    };
}
} // namespace redfish
