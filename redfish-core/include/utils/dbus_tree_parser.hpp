#pragma once
#include "dbus_to_json.hpp"

namespace redfish
{

struct DbusBaseHandler
{
    struct HeapVectors
    {
        std::vector<handler_pair> vec;
        ValueExtractor valueExtractor;
        HeapVectors(std::vector<handler_pair>&& v) :
            vec(std::move(v)), valueExtractor(makeExtractor(vec))
        {}
    };
    using handler_map =
        boost::container::flat_map<std::string, std::unique_ptr<HeapVectors>>;
    handler_map handlers;
    auto& get(const std::string& k) const
    {
        return handlers.at(k);
    }
    auto registerdHandlers() const
    {
        return handlers | std::ranges::views::keys;
    }

    void addInterfaceHandlers(const std::string& iFaceName,
                              std::vector<handler_pair>&& iFaceHandlers)
    {
        handlers.emplace(
            iFaceName, std::make_unique<HeapVectors>(std::move(iFaceHandlers)));
    }
    void addInterfaceHandler(const std::string& iFaceName,
                             std::string_view name,
                             std::unique_ptr<BaseNodeMapper> mapper)
    {
        auto iter = handlers.find(iFaceName);

        if (iter != end(handlers))
        {
            iter->second->vec.emplace_back(name, std::move(mapper));
            return;
        }
        std::vector<handler_pair> vec;
        vec.emplace_back(name, std::move(mapper));
        addInterfaceHandlers(iFaceName, std::move(vec));
    }
};

enum class DbusParserStatus
{
    Ok,
    Failed
};
template <typename Extraction_Handlers>
struct DbusTreeParser
{
    using FilterType =
        std::function<bool(const DU::ManagedObjectType::value_type&)>;
    using OnSuccessType =
        std::function<void(DbusParserStatus, const nlohmann::json&)>;
    const DU::ManagedObjectType& objects;
    const Extraction_Handlers* handlers{nullptr};

    FilterType filter;
    OnSuccessType onSuccessHandler;
    bool ignoreUnknown{false};
    DbusTreeParser(const DU::ManagedObjectType& o, Extraction_Handlers& h,
                   bool ignore_unk = false) :
        objects(o),
        handlers(&h), ignoreUnknown(ignore_unk)
    {}
    DbusTreeParser& withFilter(FilterType t)
    {
        filter = std::move(t);
        return *this;
    }

    DbusTreeParser& onSuccess(OnSuccessType h)
    {
        onSuccessHandler = std::move(h);
        return *this;
    }
    static auto selectRegisteredInterfaceHandlers(auto& reg_handlers)
    {
        return std::views::filter(
            [&](auto k) { // filter out all unregistered intefaces
            const auto& [infacename, propmap] = k;
            auto rh = reg_handlers->registerdHandlers();
            return std::ranges::find(rh, infacename) != rh.end();
        });
    }
    static auto convertInterfacedataToJson(const auto& path,
                                           auto& conv_handlers,
                                           bool ignoreUnknown)
    {
        return std::ranges::views::transform(
            [path, &conv_handlers,
             ignoreUnknown](auto k) { // convert dbus properties to json
            const auto& [ifacename, ifacedata] = k;

            return conv_handlers->get(ifacename)->valueExtractor(
                path, ifacename, ifacedata, ignoreUnknown);

        });
    }
    static nlohmann::json parseObjectProperties(const auto& path,
                                                const auto& ifaceList,
                                                const auto& handlers,
                                                bool ignoreUnknown)
    {
        nlohmann::json ifaceData;
        for (const auto& j :
             ifaceList | selectRegisteredInterfaceHandlers(handlers) |
                 convertInterfacedataToJson(path, handlers, ignoreUnknown))
        {
            if (j != nlohmann::json())
            {
                ifaceData.merge_patch(j);
            }
        }
        return ifaceData;
    }
    void parse()
    {
        try
        {
            nlohmann::json root;
            auto paths = objects |
                         std::views::filter( // if filter set for object path
                                             // use it else use identity filter
                             filter ? filter : [](auto&&) { return true; });

            for (const auto& [objectPath, interface_data] : paths)
            {
                root.merge_patch(parseObjectProperties(
                    objectPath, interface_data, handlers, ignoreUnknown));
            }
            onSuccessHandler(DbusParserStatus::Ok, root);
        }
        catch (...)
        {
            onSuccessHandler(DbusParserStatus::Failed, nlohmann::json{});
        }
    }
};
} // namespace redfish
