
#pragma once
#include <boost/container/flat_map.hpp>
#include <dbus_utility.hpp>
#include <json.hpp>

#include <functional>
#include <ranges>
#include <string_view>
#include <variant>
#include <vector>
using namespace std::ranges;
using namespace std::string_literals;
using json = nlohmann::json;
using namespace dbus::utility;
namespace redfish
{
using AssociationsValType =
    std::vector<std::tuple<std::string, std::string, std::string>>;
struct base_node_mapper
{
    virtual json visit(const DbusVariantType& var, std::string_view vw) = 0;
    virtual ~base_node_mapper() {}
};
template <typename Type>
struct node_mapper : base_node_mapper
{
    json visit(const DbusVariantType& var, std::string_view name) override
    {
        return std::visit(
            [&](auto&& val) -> json {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, Type>)
                {
                    json j;
                    j[name.data()] = val;
                    return j;
                }
                throw std::runtime_error("Wrong Type or Null Value");
            },
            var);
    }
};
template <>
struct node_mapper<AssociationsValType> : base_node_mapper
{
    json visit(const DbusVariantType& var, std::string_view name) override
    {
        return std::visit(
            [&](auto&& val) -> json {
                using T = std::decay_t<decltype(val)>;
                if constexpr (std::is_same_v<T, AssociationsValType>)
                {
                    json j;
                    // auto vw=val|views::transform([](auto&& tu){
                    //   auto [f,s,t]=tu;
                    //   return std::make_tuple(f,s,t);
                    // });

                    j[name.data()] = val;
                    return j;
                }
                throw std::runtime_error("Wrong Type or Null Value");
            },
            var);
    }
};
inline auto string_node_mapper()
{
    return std::make_unique<node_mapper<std::string>>();
};
inline auto uint32_t_node_mapper()
{
    return std::make_unique<node_mapper<uint32_t>>();
};
inline auto uint64_t_node_mapper()
{
    return std::make_unique<node_mapper<uint64_t>>();
};
inline auto int32_t_node_mapper()
{
    return std::make_unique<node_mapper<int32_t>>();
};
inline auto int64_t_node_mapper()
{
    return std::make_unique<node_mapper<int64_t>>();
};
inline auto bool_node_mapper()
{
    return std::make_unique<node_mapper<bool>>();
};
inline auto uint8_t_node_mapper()
{
    return std::make_unique<node_mapper<uint8_t>>();
};

template <typename T>
inline auto vector_node_mapper()
{
    using vec_type = std::vector<T>;
    return std::make_unique<node_mapper<vec_type>>();
};
inline auto association_val_type_node_mapper()
{
    return std::make_unique<node_mapper<AssociationsValType>>();
};

using handler_pair =
    std::pair<std::string_view, std::unique_ptr<base_node_mapper>>;
template <typename Range>
inline auto& At(const Range& cont, std::string_view name)
{
   

    auto iter = std::ranges::find_if(cont,
                                     [&](auto& e) { return e.first == name; });
    if (iter != cont.end())
    {
        return *iter;
    }
    throw std::out_of_range("Unknown Property Name");
}
using ValueExtractor = std::function<json(const DBusPropertiesMap&, bool)>;
inline auto extractFromKeys(const DBusPropertiesMap& prop_map,
                            const std::vector<handler_pair>& KeyNames,
                            bool ignore_unknown)
{
    auto json_nodes = KeyNames | std::views::transform(
                                     [&, ignore_unknown](auto& v) {
        auto& [name, mapper] = v;

        try
        {
            return mapper->visit(At(prop_map, name).second, name);
        }
        catch (std::runtime_error& e)
        {
            throw std::runtime_error(" Type conversion issue for " +
                                     std::string(name.data()));
        }
        catch (std::out_of_range& e)
        {
            if (!ignore_unknown)
            {
                throw std::runtime_error(" Missing prperty " +
                                         std::string(name.data()));
            }
        }
        json ret;
        ret[name.data()] = "null";
        return ret;
    });
    json result;
    for (auto j : json_nodes)
    {
        result.merge_patch(j);
    }
    return result;
}
auto make_extractor(std::vector<handler_pair>& keys)
{
    return [&](const DBusPropertiesMap& t, bool ignore_unknown) {
        return extractFromKeys(t, keys, ignore_unknown);
    };
}
} // namespace redfish