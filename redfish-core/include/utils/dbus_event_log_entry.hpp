#pragma once

#include "bmcweb_config.h"

#include "dbus_utility.hpp"
#include "generated/enums/log_entry.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

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

inline log_entry::LogDiagnosticDataTypes
    translateDiagnosticDataTypeDbusToRedfish(const std::string& s)
{
    if (s == "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPER")
    {
        return log_entry::LogDiagnosticDataTypes::CPER;
    }
    if (s == "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPERSection")
    {
        return log_entry::LogDiagnosticDataTypes::CPERSection;
    }
    return log_entry::LogDiagnosticDataTypes::Invalid;
}

// hasRawCper: the entry implements xyz.openbmc_project.Logging.CPER.Raw.
inline void fillEventLogCperFromPropertyMap(
    const dbus::utility::DBusPropertiesMap& resp, uint32_t entryId,
    bool hasRawCper, nlohmann::json& objectToFillOut)
{
    const std::string* diagnosticDataType = nullptr;
    const std::string* notificationType = nullptr;
    const std::string* sectionType = nullptr;

    // clang-format off
    bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), resp,
        "DiagnosticDataType", diagnosticDataType,
        "NotificationType", notificationType,
        "SectionType", sectionType
    );
    // clang-format on
    if (!success || diagnosticDataType == nullptr)
    {
        return;
    }

    log_entry::LogDiagnosticDataTypes dataType =
        translateDiagnosticDataTypeDbusToRedfish(*diagnosticDataType);
    if (dataType == log_entry::LogDiagnosticDataTypes::Invalid)
    {
        BMCWEB_LOG_WARNING("Unrecognized CPER DiagnosticDataType {}",
                           *diagnosticDataType);
        return;
    }
    objectToFillOut["DiagnosticDataType"] = dataType;

    // The raw CPER binary is served by the CPER LogService, which sits next
    // to the EventLog service and uses the same entry Id. (If dbus event log
    // interface has raw CPER interface populated then entry is created in CPER
    // LogService. The same entry Id is used to keep correlation between
    // EventLog entry and CPER LogService entry easier)
    if (hasRawCper)
    {
        std::string_view parentCollection;
        std::string_view parentMemberId;
        if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "systems")
        {
            parentCollection = "Systems";
            parentMemberId = BMCWEB_REDFISH_SYSTEM_URI_NAME;
        }
        if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
        {
            parentCollection = "Managers";
            parentMemberId = BMCWEB_REDFISH_MANAGER_URI_NAME;
        }
        nlohmann::json::object_t rawEntry;
        rawEntry["@odata.id"] = boost::urls::format(
            "/redfish/v1/{}/{}/LogServices/CPER/Entries/{}", parentCollection,
            parentMemberId, std::to_string(entryId));
        nlohmann::json::array_t relatedLogEntries;
        relatedLogEntries.emplace_back(std::move(rawEntry));

        nlohmann::json& links = objectToFillOut["Links"];
        links["RelatedLogEntries@odata.count"] = relatedLogEntries.size();
        links["RelatedLogEntries"] = std::move(relatedLogEntries);
    }

    // D-Bus already reports these as the UEFI GUID strings Redfish expects.
    // Per LogEntry schema, each GUID is only valid for its DiagnosticDataType.
    nlohmann::json::object_t cper;
    if (dataType == log_entry::LogDiagnosticDataTypes::CPER &&
        notificationType != nullptr && !notificationType->empty())
    {
        cper["NotificationType"] = *notificationType;
    }
    if (dataType == log_entry::LogDiagnosticDataTypes::CPERSection &&
        sectionType != nullptr && !sectionType->empty())
    {
        cper["SectionType"] = *sectionType;
    }

    // Read Oem separately so a wrongly typed Oem drops only Oem, not all CPER.
    const auto oemProp = std::ranges::find_if(resp, [](const auto& prop) {
        return prop.first == "Oem";
    });
    if (oemProp != resp.end())
    {
        const auto* oemDict =
            std::get_if<std::vector<std::pair<std::string, std::string>>>(
                &oemProp->second);
        if (oemDict != nullptr && !oemDict->empty())
        {
            nlohmann::json::object_t oemObj;
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
            cper["Oem"] = std::move(oemObj);
        }
        else if (oemDict == nullptr)
        {
            BMCWEB_LOG_WARNING("CPER Oem property has unexpected type");
        }
    }

    objectToFillOut["CPER"] = std::move(cper);
}
} // namespace redfish
