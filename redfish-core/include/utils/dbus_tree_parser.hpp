#pragma once
#include "dbus_to_json.hpp"

#include <sdbusplus/bus.hpp>
#define BREAK_HERE(X)                                                          \
    throw std::runtime_error(__FILE__ + std::string(",") +                     \
                             std::to_string(__LINE__) + " " + (X))
namespace redfish
{

struct DbusBaseHandler
{
    using handler_map = boost::container::flat_map<std::string, ValueExtractor>;
    handler_map handlers;
    auto& get(const std::string& k) const
    {
        return handlers.at(k);
    }
    auto registerdHandlers() const
    {
        return handlers | views::keys;
    }
};
template <typename Extraction_Handlers>
struct DbusTreeParser
{
    using FilterType =
        std::function<bool(const ManagedObjectType::value_type&)>;
    using On_ErrorType = std::function<void(const std::exception_ptr&)>;
    using On_SuccessType = std::function<void(const json&)>;
    const ManagedObjectType& objects;
    const Extraction_Handlers* handlers{nullptr};

    FilterType filter;
    On_ErrorType on_error_;
    On_SuccessType on_success_;
    bool ignore_unknown{false};
    DbusTreeParser(const ManagedObjectType& o, Extraction_Handlers& h,
                   bool ignore_unk= false) :
        objects(o),
        handlers(&h), ignore_unknown(ignore_unk)
    {}
    DbusTreeParser& with_filter(FilterType t)
    {
        filter = std::move(t);
        return *this;
    }
    DbusTreeParser& on_error(On_ErrorType h)
    {
        on_error_ = std::move(h);
        return *this;
    }

    DbusTreeParser& on_error_message(auto h)
    {
        on_error_ = [h = std::move(h)](const std::exception_ptr& e) {
            try
            {
                std::rethrow_exception(e);
            }
            catch (std::exception& ex)
            {
                h(ex.what());
            }
        };
        return *this;
    }
    DbusTreeParser& on_success(On_SuccessType h)
    {
        on_success_ = std::move(h);
        return *this;
    }
    static auto filter_registered_handlers(auto& reg_handlers)
    {
        return std::views::filter(
            [&](auto k) { // filter out all unregistered intefaces
            const auto& [infacename, propmap] = k;
            auto rh = reg_handlers->registerdHandlers();
            return std::ranges::find(rh, infacename) != rh.end();
        });
    }
    static auto iface_prop_to_json_data(auto& conv_handlers,
                                        bool ignore_unknown)
    {
        return views::transform(
            [&, ignore_unknown](auto k) { // convert dbus properties to json
            const auto& [ifacename, ifacedata] = k;

            json iface;
            iface[ifacename] = conv_handlers->get(ifacename)(ifacedata,
                                                             ignore_unknown);
            return iface;
        });
    }
    static json parseObjectProperties(const auto& ifaceList,
                                      const auto& handlers, bool ignore_unknown)
    {
        json ifaceData;
        for (const auto& j :
             ifaceList | filter_registered_handlers(handlers) |
                 iface_prop_to_json_data(handlers, ignore_unknown))
        {
            ifaceData.merge_patch(j);
        }
        return ifaceData;
    }
    void parse()
    {
        try
        {
            json root;
            auto paths = objects |
                         std::views::filter( // if filter set for object path
                                             // use it else use identity filter
                             filter ? filter : [](auto&&) { return true; });

            for (const auto& [objectPath, interface_data] : paths)
            {
                root[objectPath] = parseObjectProperties(
                    interface_data, handlers, ignore_unknown);
            }
            on_success_(root);
        }
        catch (...)
        {
            if (on_error_)
            {
                on_error_(std::current_exception());
                return;
            }
        }
    }
};
} // namespace redfish