#pragma once

#include <boost/container/flat_map.hpp>
#include <include/dbus_singleton.hpp>

#include <array>
#include <string>
#include <vector>

namespace redfish
{

namespace utils
{

enum class InventoryItemType
{
    // TODO: Support more types when proper schemas will be needed
    chassis
};

template <typename F>
inline void getInventoryItems(F&& cb)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [callback = std::move(cb)](const boost::system::error_code ec,
                                   const GetSubTreeType& tree) {
            boost::container::flat_map<std::string, InventoryItemType> items;

            if (ec)
            {
                callback(ec, items);
                return;
            }

            items.reserve(tree.size());
            for (const auto& [itemPath, serviceIfaces] : tree)
            {
                sdbusplus::message::object_path path(itemPath);
                std::string name = path.filename();
                if (name.empty())
                {
                    callback(boost::system::errc::make_error_code(
                                 boost::system::errc::invalid_argument),
                             items);
                    return;
                }

                for (const auto& [service, ifaces] : serviceIfaces)
                {
                    for (const auto& iface : ifaces)
                    {
                        std::optional<InventoryItemType> type;
                        if (iface ==
                                "xyz.openbmc_project.Inventory.Item.Board" ||
                            iface ==
                                "xyz.openbmc_project.Inventory.Item.Chassis")
                        {
                            type = InventoryItemType::chassis;
                        }

                        if (type)
                        {
                            items.emplace(name, *type);
                        }
                    }
                }
            }

            callback(ec, items);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace utils

} // namespace redfish
