#pragma once

#include "bmcweb_config.h"

#include "dbus_utility.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <unordered_map>

namespace redfish
{
struct DbusEventLogEntry
{
    // represents a subset of an instance of dbus interface
    // xyz.openbmc_project.Logging.Entry

    uint32_t Id = 0;
    std::string Message;
    const std::string* Path = nullptr;
    const std::string* Resolution = nullptr;
    bool Resolved = false;
    std::string ServiceProviderNotify;
    std::string Severity;
    uint64_t Timestamp = 0;
    uint64_t UpdateTimestamp = 0;
};

inline std::optional<DbusEventLogEntry> fillDbusEventLogEntryFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp)
{
    DbusEventLogEntry entry;

    // clang-format off
    bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "Id", entry.Id,
        "Message", entry.Message,
        "Path", entry.Path,
        "Resolution", entry.Resolution,
        "Resolved", entry.Resolved,
        "ServiceProviderNotify", entry.ServiceProviderNotify,
        "Severity", entry.Severity,
        "Timestamp", entry.Timestamp,
        "UpdateTimestamp", entry.UpdateTimestamp
    );
    // clang-format on
    if (!success)
    {
        return std::nullopt;
    }
    return entry;
}

inline std::string translateDiagnosticDataTypeDbusToRedfish(
    const std::string& s)
{
    // D-Bus enum strings are fully qualified; extract the trailing enum value.
    constexpr std::string_view prefix =
        "xyz.openbmc_project.Logging.CPER.Types.ContentType.";
    if (s.starts_with(prefix))
    {
        return s.substr(prefix.size());
    }
    return "";
}

inline void fillEventLogCperFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp,
    nlohmann::json& objectToFillOut, bool hasRawCper = false)
{
    const std::string* diagnosticDataType = nullptr;
    const std::string* notificationType = nullptr;
    const std::string* sectionType = nullptr;

    // clang-format off
    sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "DiagnosticDataType", diagnosticDataType,
        "NotificationType", notificationType,
        "SectionType", sectionType
    );
    // clang-format on

    if (diagnosticDataType == nullptr)
    {
        // Entry has no CPER diagnostic data; nothing to add.
        return;
    }

    std::string redfishType =
        translateDiagnosticDataTypeDbusToRedfish(*diagnosticDataType);
    if (redfishType.empty())
    {
        BMCWEB_LOG_WARNING(
            "Dropping CPER data: unrecognized DiagnosticDataType '{}'",
            *diagnosticDataType);
        return;
    }

    objectToFillOut["DiagnosticDataType"] = redfishType;

    // Cross-link to the raw counterpart only when the entry actually carries a
    // raw CPER artifact, signalled by the xyz.openbmc_project.Logging.CPER.Raw
    // interface (determined by the caller). CPER.Processed properties alone are
    // not sufficient; they also appear on non-CPER entries.
    auto idIt = objectToFillOut.find("Id");
    if (hasRawCper && idIt != objectToFillOut.end() && idIt->is_string())
    {
        const auto& entryId = idIt->get_ref<const std::string&>();
        // RawCPER is co-located with the EventLog, so link to the same parent.
        constexpr std::string_view rawCperParentSegment =
            (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
                ? "Managers"
                : "Systems";
        constexpr std::string_view rawCperParentId =
            (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
                ? std::string_view(BMCWEB_REDFISH_MANAGER_URI_NAME)
                : std::string_view(BMCWEB_REDFISH_SYSTEM_URI_NAME);
        nlohmann::json::array_t related;
        nlohmann::json::object_t relatedRef;
        relatedRef["@odata.id"] = boost::urls::format(
            "/redfish/v1/{}/{}/LogServices/CPERRawLog/Entries/{}",
            rawCperParentSegment, rawCperParentId, entryId);
        related.emplace_back(std::move(relatedRef));
        objectToFillOut["Links"]["RelatedLogEntries@odata.count"] =
            related.size();
        objectToFillOut["Links"]["RelatedLogEntries"] = std::move(related);
    }

    nlohmann::json& cper = objectToFillOut["CPER"];
    if (notificationType != nullptr && !notificationType->empty())
    {
        cper["NotificationType"] = *notificationType;
    }
    if (sectionType != nullptr && !sectionType->empty())
    {
        cper["SectionType"] = *sectionType;
    }

    // Oem is a{ss} — unpack can't resolve this variant; search directly.
    const auto oemProp = std::ranges::find_if(resp, [](const auto& prop) {
        return prop.first == "Oem";
    });
    if (oemProp == resp.end())
    {
        return;
    }

    nlohmann::json& oemObj = cper["Oem"];
    // Oem is a{ss} — unpack can't resolve this variant; search directly.
    for (const auto& [key, value] : resp)
    {
        if (key != "Oem")
        {
            continue;
        }
        const auto* oemDict =
            std::get_if<std::unordered_map<std::string, std::string>>(&value);
        if (oemDict != nullptr && !oemDict->empty())
        {
            for (const auto& [k, v] : *oemDict)
            {
                auto parsed = nlohmann::json::parse(v, nullptr, false);
                if (!parsed.is_discarded())
                {
                    oemObj[k] = std::move(parsed);
                }
                else
                {
                    oemObj[k] = v;
                }
            }
        }
        break;
    }
}
} // namespace redfish
