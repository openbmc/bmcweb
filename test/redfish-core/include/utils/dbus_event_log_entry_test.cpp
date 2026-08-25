// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "bmcweb_config.h"

#include "dbus_utility.hpp"
#include "generated/enums/log_entry.hpp"
#include "utils/dbus_event_log_entry.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using nlohmann::literals::operator""_json;
using OemDict = std::vector<std::pair<std::string, std::string>>;

constexpr const char* cperDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPER";
constexpr const char* cperSectionDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPERSection";
// UEFI "Machine Check Exception" notification type GUID
constexpr const char* notificationGuid = "E8F56FFE-919C-4CC5-BA88-65ABE14913BB";
// UEFI "Processor Generic" section type GUID
constexpr const char* sectionGuid = "9876CCAD-47B4-4BDB-B65E-16F193C4F3DB";

// Expected Links/RelatedLogEntries target for the configured EventLog parent.
std::string expectedRawCperUri(uint32_t entryId)
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
    return std::format("/redfish/v1/{}/{}/LogServices/CPER/Entries/{}",
                       parentCollection, parentMemberId, entryId);
}

dbus::utility::DBusPropertiesMap makeCperProps(
    std::string_view diagnosticDataType, std::string_view notificationType,
    std::string_view sectionType)
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", std::string(diagnosticDataType));
    props.emplace_back("NotificationType", std::string(notificationType));
    props.emplace_back("SectionType", std::string(sectionType));
    return props;
}

dbus::utility::DBusPropertiesMap makeEntryProps()
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Id", uint32_t{42});
    props.emplace_back("Message", std::string("msg"));
    props.emplace_back("Resolved", true);
    props.emplace_back(
        "ServiceProviderNotify",
        std::string("xyz.openbmc_project.Logging.Entry.Notify.Notify"));
    props.emplace_back(
        "Severity",
        std::string("xyz.openbmc_project.Logging.Entry.Level.Error"));
    props.emplace_back("Timestamp", uint64_t{1000});
    props.emplace_back("UpdateTimestamp", uint64_t{2000});
    return props;
}

TEST(TranslateDiagnosticDataTypeDbusToRedfish, CperMapsToCper)
{
    EXPECT_EQ(translateDiagnosticDataTypeDbusToRedfish(std::string(cperDbus)),
              log_entry::LogDiagnosticDataTypes::CPER);
}

TEST(TranslateDiagnosticDataTypeDbusToRedfish, CperSectionMapsToCperSection)
{
    EXPECT_EQ(
        translateDiagnosticDataTypeDbusToRedfish(std::string(cperSectionDbus)),
        log_entry::LogDiagnosticDataTypes::CPERSection);
}

TEST(TranslateDiagnosticDataTypeDbusToRedfish, UnknownValuesMapToInvalid)
{
    const std::vector<std::string> invalid = {
        "",
        "CPER",
        "xyz.openbmc_project.Logging.CPER.Types.ContentType.cper",
        "xyz.openbmc_project.Logging.CPER.Types.ContentType.OEM",
    };
    for (const std::string& s : invalid)
    {
        EXPECT_EQ(translateDiagnosticDataTypeDbusToRedfish(s),
                  log_entry::LogDiagnosticDataTypes::Invalid)
            << "input: '" << s << "'";
    }
}

// The EventLog flattens every interface, so CPER props and Oem must not
// break parsing of the base Logging.Entry fields.
TEST(FillDbusEventLogEntryFromPropertyMap, CperPropertiesAndOemAreIgnored)
{
    dbus::utility::DBusPropertiesMap props = makeEntryProps();
    props.emplace_back("DiagnosticDataType", std::string(cperDbus));
    props.emplace_back("NotificationType", std::string(notificationGuid));
    props.emplace_back("SectionType", std::string(sectionGuid));
    props.emplace_back("Oem", OemDict{{"k", "v"}});
    std::optional<DbusEventLogEntry> entry =
        fillDbusEventLogEntryFromPropertyMap(props);
    ASSERT_TRUE(entry);
    if (!entry)
    {
        return;
    }
    EXPECT_EQ(entry->Id, 42U);
}

TEST(FillEventLogCperFromPropertyMap, EmptyPropertyMapLeavesJsonUntouched)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    fillEventLogCperFromPropertyMap({}, 1, true, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

TEST(FillEventLogCperFromPropertyMap, NonCperEntryLeavesJsonUntouched)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    dbus::utility::DBusPropertiesMap props = makeEntryProps();
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

// Without DiagnosticDataType nothing is emitted, even if CPER.Raw is present
// or other CPER properties / Oem are populated.
TEST(FillEventLogCperFromPropertyMap, MissingDiagnosticDataTypeIgnoresRest)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("NotificationType", std::string(notificationGuid));
    props.emplace_back("SectionType", std::string(sectionGuid));
    props.emplace_back("Oem", OemDict{{"k", "v"}});
    fillEventLogCperFromPropertyMap(props, 1, true, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

TEST(FillEventLogCperFromPropertyMap, DiagnosticDataTypeWrongTypeIgnored)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", uint32_t{1});
    fillEventLogCperFromPropertyMap(props, 1, true, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

// unpackPropertiesNoThrow is all-or-nothing, so a bad GUID type drops CPER.
TEST(FillEventLogCperFromPropertyMap, NotificationTypeWrongTypeIgnored)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", std::string(cperDbus));
    props.emplace_back("NotificationType", uint32_t{1});
    fillEventLogCperFromPropertyMap(props, 1, true, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

TEST(FillEventLogCperFromPropertyMap, UnknownDiagnosticDataTypeIgnored)
{
    nlohmann::json j = R"({"Id": "1"})"_json;
    dbus::utility::DBusPropertiesMap props =
        makeCperProps("xyz.openbmc_project.Logging.CPER.Types.ContentType.OEM",
                      notificationGuid, sectionGuid);
    props.emplace_back("Oem", OemDict{{"k", "v"}});
    fillEventLogCperFromPropertyMap(props, 1, true, j);
    EXPECT_EQ(j, R"({"Id": "1"})"_json);
}

TEST(FillEventLogCperFromPropertyMap, CperOnlyEmitsEmptyCperObject)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", std::string(cperDbus));
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j, R"({"DiagnosticDataType": "CPER", "CPER": {}})"_json);
}

TEST(FillEventLogCperFromPropertyMap, CperWithNotificationType)
{
    nlohmann::json j = nlohmann::json::object();
    fillEventLogCperFromPropertyMap(
        makeCperProps(cperDbus, notificationGuid, ""), 1, false, j);
    EXPECT_EQ(j["DiagnosticDataType"], "CPER");
    EXPECT_EQ(j["CPER"]["NotificationType"], notificationGuid);
    EXPECT_FALSE(j["CPER"].contains("SectionType"));
    EXPECT_FALSE(j["CPER"].contains("Oem"));
    EXPECT_FALSE(j.contains("Links"));
}

TEST(FillEventLogCperFromPropertyMap, CperSectionOmitsNotificationType)
{
    nlohmann::json j = nlohmann::json::object();
    fillEventLogCperFromPropertyMap(
        makeCperProps(cperSectionDbus, notificationGuid, sectionGuid), 1, false,
        j);
    EXPECT_EQ(j["DiagnosticDataType"], "CPERSection");
    EXPECT_EQ(j["CPER"], nlohmann::json({{"SectionType", sectionGuid}}));
}

TEST(FillEventLogCperFromPropertyMap, EmptyGuidsAreOmitted)
{
    nlohmann::json j = nlohmann::json::object();
    fillEventLogCperFromPropertyMap(makeCperProps(cperSectionDbus, "", ""), 1,
                                    false, j);
    EXPECT_EQ(j["DiagnosticDataType"], "CPERSection");
    EXPECT_EQ(j["CPER"], nlohmann::json::object());
}

// DiagnosticDataType is not inferred from SectionType; SectionType is dropped.
TEST(FillEventLogCperFromPropertyMap, CperTypeOmitsSectionType)
{
    nlohmann::json j = nlohmann::json::object();
    fillEventLogCperFromPropertyMap(
        makeCperProps(cperDbus, notificationGuid, sectionGuid), 1, false, j);
    EXPECT_EQ(j["DiagnosticDataType"], "CPER");
    EXPECT_EQ(j["CPER"],
              nlohmann::json({{"NotificationType", notificationGuid}}));
}

TEST(FillEventLogCperFromPropertyMap, RawCperAddsRelatedLogEntry)
{
    nlohmann::json j = nlohmann::json::object();
    fillEventLogCperFromPropertyMap(
        makeCperProps(cperDbus, notificationGuid, ""), 5, true, j);
    ASSERT_TRUE(j.contains("Links"));
    EXPECT_EQ(j["Links"]["RelatedLogEntries@odata.count"], 1);
    ASSERT_TRUE(j["Links"]["RelatedLogEntries"].is_array());
    ASSERT_EQ(j["Links"]["RelatedLogEntries"].size(), 1U);
    EXPECT_EQ(j["Links"]["RelatedLogEntries"][0]["@odata.id"],
              expectedRawCperUri(5));
    EXPECT_EQ(j["Links"]["RelatedLogEntries"][0].size(), 1U);
}

TEST(FillEventLogCperFromPropertyMap, OemPlainStrings)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props = makeCperProps(cperDbus, "", "");
    props.emplace_back("Oem", OemDict{{"Vendor", "Acme"}, {"Model", "X 1"}});
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j["CPER"]["Oem"], R"({"Vendor": "Acme", "Model": "X 1"})"_json);
}

TEST(FillEventLogCperFromPropertyMap, OemJsonValuesAreParsed)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props = makeCperProps(cperDbus, "", "");
    props.emplace_back("Oem", OemDict{
                                  {"Obj", R"({"a": 1, "b": [true, "x"]})"},
                                  {"Arr", "[1, 2, 3]"},
                                  {"Num", "123"},
                                  {"Bool", "true"},
                                  {"Str", R"("quoted")"},
                              });
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j["CPER"]["Oem"], R"({
        "Obj": {"a": 1, "b": [true, "x"]},
        "Arr": [1, 2, 3],
        "Num": 123,
        "Bool": true,
        "Str": "quoted"
    })"_json);
}

TEST(FillEventLogCperFromPropertyMap, OemInvalidJsonFallsBackToString)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props = makeCperProps(cperDbus, "", "");
    props.emplace_back("Oem", OemDict{
                                  {"Empty", ""},
                                  {"Truncated", R"({"a": 1)"},
                                  {"Bare", "hello world"},
                              });
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j["CPER"]["Oem"]["Empty"], "");
    EXPECT_EQ(j["CPER"]["Oem"]["Truncated"], R"({"a": 1)");
    EXPECT_EQ(j["CPER"]["Oem"]["Bare"], "hello world");
}

TEST(FillEventLogCperFromPropertyMap, OemEmptyDictIsOmitted)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props = makeCperProps(cperDbus, "", "");
    props.emplace_back("Oem", OemDict{});
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j["CPER"], nlohmann::json::object());
}

TEST(FillEventLogCperFromPropertyMap, OemWrongTypeIsOmitted)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props = makeCperProps(cperDbus, "", "");
    props.emplace_back("Oem", std::string(R"({"k": "v"})"));
    fillEventLogCperFromPropertyMap(props, 1, false, j);
    EXPECT_EQ(j["DiagnosticDataType"], "CPER");
    EXPECT_EQ(j["CPER"], nlohmann::json::object());
}

TEST(FillEventLogCperFromPropertyMap, FullCperSectionEntry)
{
    nlohmann::json j = R"({"Id": "12"})"_json;
    dbus::utility::DBusPropertiesMap props =
        makeCperProps(cperSectionDbus, notificationGuid, sectionGuid);
    props.emplace_back("Oem", OemDict{{"Decoded", R"({"Severity": "Fatal"})"},
                                      {"Socket", "0"}});
    fillEventLogCperFromPropertyMap(props, 12, true, j);

    nlohmann::json expected = R"({
        "Id": "12",
        "DiagnosticDataType": "CPERSection",
        "CPER": {
            "SectionType": "9876CCAD-47B4-4BDB-B65E-16F193C4F3DB",
            "Oem": {"Decoded": {"Severity": "Fatal"}, "Socket": 0}
        },
        "Links": {"RelatedLogEntries@odata.count": 1}
    })"_json;
    nlohmann::json::object_t related;
    related["@odata.id"] = expectedRawCperUri(12);
    expected["Links"]["RelatedLogEntries"] = nlohmann::json::array({related});
    EXPECT_EQ(j, expected);
}

} // namespace
} // namespace redfish
