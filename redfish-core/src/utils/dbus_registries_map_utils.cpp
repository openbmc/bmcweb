#include "utils/dbus_registries_map_utils.hpp"

#include "external_registries_map.hpp"
#include "utils/dbus_event_log_entry.hpp"

#include <format>
#include <string_view>

namespace redfish
{
namespace dbus_registries_map
{
std::optional<MappedInfo> getMappedInfo(const DbusEventLogEntry& entry)
{
    auto message = entry.Message.c_str();
    auto it = std::find_if(dbusToRedfishMessageId.begin(),
                           dbusToRedfishMessageId.end(),
                           [&message](const auto& element) {
                               return std::strcmp(element.first, message) == 0;
                           });

    if (it == dbusToRedfishMessageId.end())
    {
        return std::nullopt;
    }

    auto dbusToRfInfo = it->second;
    // Currently we don't use message version, so set it to 0.0
    std::string messageId =
        std::format("{}.0.0.{}", std::string_view(dbusToRfInfo.registryName),
                    std::string_view(dbusToRfInfo.redfishMessageId));

    std::vector<std::string> args = {};

    if (dbusToRfInfo.numberOfArgs > 0)
    {
        for (size_t index = 0; index < dbusToRfInfo.numberOfArgs; index++)
        {
            auto additionalDataElement = entry.AdditionalData.find(
                std::string(dbusToRfInfo.argsInfo[index].name));

            if (additionalDataElement == entry.AdditionalData.end())
            {
                BMCWEB_LOG_WARNING(
                    "Could not find expected arg '{}' in additional data",
                    dbusToRfInfo.argsInfo[index].name);
                continue;
            }
            args.push_back(additionalDataElement->second);
        }
    }
    return MappedInfo{messageId, args};
}
} // namespace dbus_registries_map
} // namespace redfish
