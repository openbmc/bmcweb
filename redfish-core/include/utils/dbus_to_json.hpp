
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

namespace dbus
{
namespace utility
{
enum class MapperResult
{
    Ok,
    TypeMissMatch,
    NotFound
};
struct MetaData
{
    nlohmann::json res;
};
struct BaseNodeMapper
{
    virtual std::optional<nlohmann::json>
        visit(const DbusVariantType& var,
              const sdbusplus::message::object_path&,
              std::string_view ifaceName, std::string_view nm,
              MetaData& metaData) = 0;
    BaseNodeMapper() = default;
    BaseNodeMapper(const BaseNodeMapper&) = delete;
    BaseNodeMapper& operator=(const BaseNodeMapper&) = delete;
    BaseNodeMapper(BaseNodeMapper&&) = default;
    BaseNodeMapper& operator=(BaseNodeMapper&&) = default;
    virtual ~BaseNodeMapper() = default;
};
template <typename Type>
concept ConvertibleToJson_V = std::is_convertible_v<Type, nlohmann::json>;
template <typename Type>
concept IsSameJson_V = std::is_same_v<Type, nlohmann::json>;
template <class Type, typename First, typename Second>
concept ErrorReturn = std::same_as<Type, std::pair<First, Second>> &&
                      ConvertibleToJson_V<First> && IsSameJson_V<Second>;
template <typename Handler, typename Val>
concept InvokableWithPath = requires(Handler h, const Val& v,
                                     const sdbusplus::message::object_path& p) {
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

template <typename Handler, typename EnumType>
concept InvokableWithStringReturnsEnum =
    requires(Handler h, const std::string& s) {
        {
            h(s)
        } -> std::convertible_to<EnumType>;
    };

template <typename Handler, typename Val>
concept InvokableWithPathAndError =
    requires(Handler h, const Val& v, const sdbusplus::message::object_path& p,
             MapperResult& mr, MetaData& md) {
        {
            h(p, v, mr, md)
        } -> std::convertible_to<nlohmann::json>;
    };
template <typename Handler, typename Val>
concept InvokableWithValueAndError =
    requires(Handler h, const Val& v, MapperResult& mr, MetaData& md) {
        {
            h(v, mr, md)
        } -> std::convertible_to<nlohmann::json>;
    };
template <typename Handler, typename Val>
concept InvokableWithIfaceNameAndError =
    requires(Handler h, const Val& v, std::string_view vw, MapperResult& mr,
             MetaData& md) {
        {
            h(vw, v, mr, md)
        } -> std::convertible_to<nlohmann::json>;
    };
template <typename Handler, typename Val>
concept InvokableWithPathAndIfaceNameAndError =
    requires(Handler h, const Val& v, const sdbusplus::message::object_path& p,
             std::string_view vw, MapperResult& mr, MetaData& md) {
        {
            h(p, vw, v, mr, md)
        } -> std::convertible_to<nlohmann::json>;
    };
template <typename Handler, typename EnumType>
concept InvokableWithStringReturnsEnumAndError =
    requires(Handler h, const std::string& s, MapperResult& mr, MetaData& md) {
        {
            h(s, mr, md)
        } -> std::convertible_to<nlohmann::json>;
    };
using AssociationsValType =
    std::vector<std::tuple<std::string, std::string, std::string>>;

template <typename Type>
using MapperHandler = std::function<nlohmann::json(
    const sdbusplus::message::object_path&, std::string_view, std::string_view,
    const Type&)>;
template <typename Type>
using MapperHandlerWithError = std::function<nlohmann::json(
    const sdbusplus::message::object_path&, std::string_view, std::string_view,
    const Type&, MapperResult&, MetaData&)>;
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
    explicit GenericNodeMapper(const std::string& k) : key(k) {}
    std::optional<nlohmann::json>
        visit(const DbusVariantType& var,
              const sdbusplus::message::object_path& /*unused*/,
              std::string_view /*unused*/, std::string_view /*unused*/,
              MetaData& /*unused*/) override
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
    explicit NodeMapper(HandlerType h) : handler(std::move(h)) {}
    std::optional<nlohmann::json>
        visit(const DbusVariantType& var,
              const sdbusplus::message::object_path& path,
              std::string_view ifaceName, std::string_view name,
              MetaData& /*unused*/) override
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
struct NodeMapperWithError : BaseNodeMapper
{
    using HandlerType = MapperHandlerWithError<Type>;
    HandlerType handler;
    explicit NodeMapperWithError(HandlerType h) : handler(std::move(h)) {}
    std::optional<nlohmann::json>
        visit(const DbusVariantType& var,
              const sdbusplus::message::object_path& path,
              std::string_view ifaceName, std::string_view name,
              MetaData& metaData) override
    {
        return std::visit(
            [&](auto&& val) -> std::optional<nlohmann::json> {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, Type>)
                {
                    MapperResult mr = MapperResult::Ok;
                    auto result = handler(path, ifaceName, name, val, mr,
                                          metaData);
                    if (mr == MapperResult::Ok)
                    {
                        return result;
                    }
                }
                else
                {
                    MapperResult mr = MapperResult::TypeMissMatch;
                    auto result = handler(path, ifaceName, name, Type{}, mr,
                                          metaData);
                    if (mr == MapperResult::Ok)
                    {
                        return result;
                    }
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
template <typename Type>
inline auto makeNodeMapper(MapperHandlerWithError<Type> handler)
{
    return std::make_unique<NodeMapperWithError<Type>>(std::move(handler));
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
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](const sdbusplus::message::object_path&,
                                std::string_view, std::string_view,
                                const Type& val) {
        return makeJson(key, h(val));
    });
}

template <typename Type, InvokableWithPath<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](const sdbusplus::message::object_path& path,
                                std::string_view, std::string_view,
                                const Type& val) {
        return makeJson(key, h(path, val));
    });
}

template <typename Type, InvokableWithIfaceName<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](const sdbusplus::message::object_path&,
                                std::string_view iFaceName, std::string_view,
                                const Type& val) {
        return makeJson(key, h(iFaceName, val));
    });
}

template <typename Type, InvokableWithPathAndIfaceName<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](const sdbusplus::message::object_path& path,
                                std::string_view ifaceName, std::string_view,
                                const Type& val) {
        return makeJson(key, h(path, ifaceName, val));
    });
}

template <typename Type, InvokableWithValueAndError<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h),
         key](const sdbusplus::message::object_path&, std::string_view,
              std::string_view, const Type& val, MapperResult& mr,
              MetaData& md) { return makeJson(key, h(val, mr, md)); });
}

template <typename Type, InvokableWithPathAndError<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h),
         key](const sdbusplus::message::object_path& path, std::string_view,
              std::string_view, const Type& val, MapperResult& mr,
              MetaData& md) { return makeJson(key, h(path, val, mr, md)); });
}

template <typename Type, InvokableWithIfaceNameAndError<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](
            const sdbusplus::message::object_path&, std::string_view iFaceName,
            std::string_view, const Type& val, MapperResult& mr, MetaData& md) {
        return makeJson(key, h(iFaceName, val, mr, md));
    });
}

template <typename Type, InvokableWithPathAndIfaceNameAndError<Type> Handler>
inline auto mapToKey(const std::string& key, Handler h)
{
    return makeNodeMapper<Type>(
        [h = std::move(h), key](const sdbusplus::message::object_path& path,
                                std::string_view ifaceName, std::string_view,
                                const Type& val, MapperResult& mr,
                                MetaData& md) {
        return makeJson(key, h(path, ifaceName, val, mr, md));
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
template <typename Type, InvokableWithPath<Type> Handler>
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
template <typename Enum, InvokableWithStringReturnsEnum<Enum> Handler>
inline auto mapToEnumKey(const std::string& key, Handler handler)
{
    auto enumMapper = [key, handler = std::move(handler)](const auto& v) {
        nlohmann::json j;
        j[key] = handler(v);
        return j;
    };
    return mapToHandler<std::string>(std::move(enumMapper));
}
using handler_pair =
    std::pair<std::string_view, std::unique_ptr<BaseNodeMapper>>;

using ValueExtractor = std::function<nlohmann::json(
    const sdbusplus::message::object_path&, std::string_view,
    const DBusPropertiesMap&, bool, MetaData&)>;

std::optional<nlohmann::json>
    visitIfPresent(const auto& path, const auto& ifaceName, const auto& propMap,
                   const auto& mapper, const auto& name, auto& metaData)
{
    auto iter = std::ranges::find_if(propMap,
                                     [&](auto& e) { return e.first == name; });
    if (iter != propMap.end())
    {
        return mapper->visit(iter->second, path, ifaceName, name, metaData);
    }
    return mapper->visit(DbusVariantType(), path, ifaceName, name, metaData);
}
inline auto extractFromInterfaceKeys(const auto& path, const auto& ifaceName,
                                     const DBusPropertiesMap& propMap,
                                     const std::vector<handler_pair>& keyNames,
                                     bool ignoreUnknown, auto& metaData)
{
    auto jsonNodes = keyNames |
                     std::views::transform(
                         [&propMap, &path, &ifaceName,
                          &metaData](auto& v) -> std::optional<nlohmann::json> {
                             auto& [name, mapper] = v;
                             return visitIfPresent(path, ifaceName, propMap,
                                                   mapper, name, metaData);
                         }) |
                     std::views::filter([ignoreUnknown](const auto& v) {
                         return ignoreUnknown ? v.has_value() : true;
                     });
    nlohmann::json result;
    for (auto j : jsonNodes)
    {
        result.merge_patch(j.value());
    }
    return result;
}
inline auto makeExtractor(std::vector<handler_pair>& keys)
{
    return [&keys](const auto& path, const auto& ifaceName,
                   const DBusPropertiesMap& propMap, bool ignoreUnknown,
                   MetaData& metaData) {
        return extractFromInterfaceKeys(path, ifaceName, propMap, keys,
                                        ignoreUnknown, metaData);
    };
}
} // namespace utility

} // namespace dbus
