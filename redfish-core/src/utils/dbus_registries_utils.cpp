#include "utils/dbus_registries_utils.hpp"

#include "dbus_registries_map.hpp"

namespace redfish
{
namespace dbus_registries
{
std::optional<std::pair<std::string, std::vector<std::string>>>
    getRfMessageIdAndArgs(const DbusEventLogEntry& entry)
{
    std::string messageToSearch;
    if (entry.Message.starts_with(
            redfish::dbus_registries_map::dbusNamespacePrefix))
    {
        messageToSearch = entry.Message.substr(
            redfish::dbus_registries_map::dbusNamespacePrefix.length());
    }
    else
    {
        return std::nullopt;
    }

    if (auto dbusToRfInfo =
            redfish::dbus_registries_map::dbusToRedfishMessageId.find(
                messageToSearch);
        dbusToRfInfo !=
        redfish::dbus_registries_map::dbusToRedfishMessageId.end())
    {
        // Currently we don't use message version, so set it to 0.0
        std::string messageId =
            std::format("{}.0.0.{}", dbusToRfInfo->second.RegistryName,
                        dbusToRfInfo->second.RedfishMessageId);

        std::vector<std::string> args = {};
        for (const auto& expectedArgInfo : dbusToRfInfo->second.ArgsInfo)
        {
            auto it =
                entry.AdditionalData.find(std::string(expectedArgInfo.first));

            if (it == entry.AdditionalData.end())
            {
                BMCWEB_LOG_WARNING(
                    "Could not find expected arg '{}' in additional data",
                    expectedArgInfo.first);
                continue;
            }
            args.push_back(it->second);
        }
        return std::make_pair(messageId, args);
    }

    return std::nullopt;
}
} // namespace dbus_registries
} // namespace redfish
