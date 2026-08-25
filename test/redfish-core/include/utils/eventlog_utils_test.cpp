// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "utils/eventlog_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstdint>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish::eventlog_utils
{
namespace
{

using OemDict = std::vector<std::pair<std::string, std::string>>;

constexpr const char* entryIface = "xyz.openbmc_project.Logging.Entry";
constexpr const char* processedIface =
    "xyz.openbmc_project.Logging.CPER.Processed";
constexpr const char* rawIface = "xyz.openbmc_project.Logging.CPER.Raw";
constexpr const char* cperDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPER";
constexpr const char* cperSectionDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPERSection";
constexpr const char* notificationGuid = "E8F56FFE-919C-4CC5-BA88-65ABE14913BB";
constexpr const char* sectionGuid = "9876CCAD-47B4-4BDB-B65E-16F193C4F3DB";

constexpr const char* collectionStr = "Systems";
constexpr const char* logEntryDescriptor = "System";
constexpr std::string_view memberId = "system";

boost::system::error_code ebadr()
{
    return {EBADR, boost::system::system_category()};
}

boost::system::error_code makeEc(boost::system::errc::errc_t e)
{
    return boost::system::errc::make_error_code(e);
}

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

dbus::utility::DBusPropertiesMap makeEntryProps(uint32_t id)
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("Id", id);
    props.emplace_back("Message", std::format("message {}", id));
    props.emplace_back("Resolved", false);
    props.emplace_back(
        "ServiceProviderNotify",
        std::string("xyz.openbmc_project.Logging.Entry.Notify.Inhibit"));
    props.emplace_back(
        "Severity",
        std::string("xyz.openbmc_project.Logging.Entry.Level.Error"));
    props.emplace_back("Timestamp", uint64_t{1700000000000});
    props.emplace_back("UpdateTimestamp", uint64_t{1700000001000});
    return props;
}

dbus::utility::DBusPropertiesMap makeCperProps(const std::string& type,
                                               const std::string& notification,
                                               const std::string& section)
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", type);
    props.emplace_back("NotificationType", notification);
    props.emplace_back("SectionType", section);
    return props;
}

// Builds one managed object. CPER properties live on a separate interface to
// prove they are flattened together with the Logging.Entry properties.
std::pair<sdbusplus::object_path, dbus::utility::DBusInterfacesMap> makeObject(
    uint32_t id, bool cper, bool raw)
{
    std::pair<sdbusplus::object_path, dbus::utility::DBusInterfacesMap> obj;
    obj.first = sdbusplus::object_path("/xyz/openbmc_project/logging/entry") /
                std::to_string(id);
    obj.second.emplace_back(entryIface, makeEntryProps(id));
    if (cper)
    {
        dbus::utility::DBusPropertiesMap cperProps =
            makeCperProps(cperDbus, notificationGuid, "");
        cperProps.emplace_back("Oem", OemDict{{"Socket", "1"}});
        obj.second.emplace_back(processedIface, std::move(cperProps));
    }
    if (raw)
    {
        obj.second.emplace_back(rawIface, dbus::utility::DBusPropertiesMap{});
    }
    return obj;
}

void expectNotFound(crow::Response& res, const std::string& type,
                    const std::string& id)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(res.jsonValue["error"]["@Message.ExtendedInfo"][0]["MessageArgs"],
              nlohmann::json::array({type, id}));
}

void expectEmptyCollection(crow::Response& res)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(res.jsonValue["Members@odata.count"], 0);
    EXPECT_EQ(res.jsonValue["Members"], nlohmann::json::array());
    EXPECT_FALSE(res.jsonValue.contains("error"));
}

TEST(AfterLogEntriesGetManagedObjects, EbadrReturnsEmptyCollection)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, ebadr(), {});
    expectEmptyCollection(asyncResp->res);
}

TEST(AfterLogEntriesGetManagedObjects, HostUnreachableReturnsEmptyCollection)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(
        asyncResp, collectionStr, memberId, logEntryDescriptor,
        makeEc(boost::system::errc::host_unreachable), {});
    expectEmptyCollection(asyncResp->res);
}

TEST(AfterLogEntriesGetManagedObjects, OtherErrorsReturnInternalError)
{
    for (boost::system::errc::errc_t e :
         {boost::system::errc::timed_out, boost::system::errc::io_error,
          boost::system::errc::permission_denied,
          boost::system::errc::invalid_argument})
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                         logEntryDescriptor, makeEc(e), {});
        EXPECT_EQ(asyncResp->res.result(),
                  boost::beast::http::status::internal_server_error)
            << "errc: " << static_cast<int>(e);
    }
}

TEST(AfterLogEntriesGetManagedObjects, NoObjectsReturnsEmptyCollection)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, {});
    expectEmptyCollection(asyncResp->res);
}

TEST(AfterLogEntriesGetManagedObjects, ObjectsWithoutEntryIfaceAreSkipped)
{
    dbus::utility::ManagedObjectType resp;
    auto& obj = resp.emplace_back();
    obj.first = sdbusplus::object_path("/xyz/openbmc_project/logging");
    obj.second.emplace_back("xyz.openbmc_project.Collection.DeleteAll",
                            dbus::utility::DBusPropertiesMap{});
    // A Raw-only object (no Logging.Entry) is not an EventLog entry.
    auto& rawOnly = resp.emplace_back();
    rawOnly.first =
        sdbusplus::object_path("/xyz/openbmc_project/logging/entry/5");
    rawOnly.second.emplace_back(rawIface, dbus::utility::DBusPropertiesMap{});
    rawOnly.second.emplace_back(processedIface,
                                makeCperProps(cperDbus, notificationGuid, ""));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    expectEmptyCollection(asyncResp->res);
}

TEST(AfterLogEntriesGetManagedObjects, InvalidEntryReturnsInternalError)
{
    dbus::utility::ManagedObjectType resp;
    auto& obj = resp.emplace_back();
    obj.first = sdbusplus::object_path("/xyz/openbmc_project/logging/entry/1");
    dbus::utility::DBusPropertiesMap props = makeEntryProps(1);
    props[0].second = std::string("not-a-uint32");
    obj.second.emplace_back(entryIface, std::move(props));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterLogEntriesGetManagedObjects, NonCperEntryHasNoCperFields)
{
    dbus::utility::ManagedObjectType resp;
    resp.emplace_back(makeObject(1, false, false));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1U);
    EXPECT_EQ(members[0]["Id"], "1");
    EXPECT_EQ(members[0]["@odata.id"],
              "/redfish/v1/Systems/system/LogServices/EventLog/Entries/1");
    EXPECT_EQ(members[0]["Name"], "System Event Log Entry");
    EXPECT_EQ(members[0]["Severity"], "Critical");
    EXPECT_FALSE(members[0].contains("DiagnosticDataType"));
    EXPECT_FALSE(members[0].contains("CPER"));
    EXPECT_FALSE(members[0].contains("Links"));
}

TEST(AfterLogEntriesGetManagedObjects, CperEntryWithoutRawHasNoLinks)
{
    dbus::utility::ManagedObjectType resp;
    resp.emplace_back(makeObject(3, true, false));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1U);
    EXPECT_EQ(members[0]["DiagnosticDataType"], "CPER");
    EXPECT_EQ(members[0]["CPER"]["NotificationType"], notificationGuid);
    EXPECT_EQ(members[0]["CPER"]["Oem"]["Socket"], 1);
    EXPECT_FALSE(members[0].contains("Links"));
}

TEST(AfterLogEntriesGetManagedObjects, CperEntryWithRawHasRelatedLogEntry)
{
    dbus::utility::ManagedObjectType resp;
    resp.emplace_back(makeObject(3, true, true));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1U);
    EXPECT_EQ(members[0]["DiagnosticDataType"], "CPER");
    EXPECT_EQ(members[0]["Links"]["RelatedLogEntries@odata.count"], 1);
    EXPECT_EQ(members[0]["Links"]["RelatedLogEntries"][0]["@odata.id"],
              expectedRawCperUri(3));
}

// The Raw interface alone does not make an entry a CPER entry.
TEST(AfterLogEntriesGetManagedObjects, RawWithoutDiagnosticDataTypeNoLinks)
{
    dbus::utility::ManagedObjectType resp;
    resp.emplace_back(makeObject(4, false, true));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1U);
    EXPECT_FALSE(members[0].contains("DiagnosticDataType"));
    EXPECT_FALSE(members[0].contains("CPER"));
    EXPECT_FALSE(members[0].contains("Links"));
}

TEST(AfterLogEntriesGetManagedObjects, CperPropertiesOnEntryIfaceAlsoWork)
{
    dbus::utility::ManagedObjectType resp;
    auto& obj = resp.emplace_back();
    obj.first = sdbusplus::object_path("/xyz/openbmc_project/logging/entry/8");
    dbus::utility::DBusPropertiesMap props = makeEntryProps(8);
    props.emplace_back("DiagnosticDataType", std::string(cperSectionDbus));
    props.emplace_back("SectionType", std::string(sectionGuid));
    obj.second.emplace_back(entryIface, std::move(props));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 1U);
    EXPECT_EQ(members[0]["DiagnosticDataType"], "CPERSection");
    EXPECT_EQ(members[0]["CPER"]["SectionType"], sectionGuid);
}

// Per-entry state (hasRawCper, CPER fields) must not leak between entries,
// and the result is sorted numerically by Id.
TEST(AfterLogEntriesGetManagedObjects, MixedEntriesAreIsolatedAndSorted)
{
    dbus::utility::ManagedObjectType resp;
    resp.emplace_back(makeObject(10, true, false));
    resp.emplace_back(makeObject(2, true, true));
    resp.emplace_back(makeObject(1, false, false));
    resp.emplace_back(makeObject(100, false, true));

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterLogEntriesGetManagedObjects(asyncResp, collectionStr, memberId,
                                     logEntryDescriptor, {}, resp);
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 4);
    nlohmann::json& members = asyncResp->res.jsonValue["Members"];
    ASSERT_EQ(members.size(), 4U);

    EXPECT_EQ(members[0]["Id"], "1");
    EXPECT_FALSE(members[0].contains("CPER"));
    EXPECT_FALSE(members[0].contains("Links"));

    EXPECT_EQ(members[1]["Id"], "2");
    EXPECT_EQ(members[1]["DiagnosticDataType"], "CPER");
    EXPECT_EQ(members[1]["Links"]["RelatedLogEntries"][0]["@odata.id"],
              expectedRawCperUri(2));

    EXPECT_EQ(members[2]["Id"], "10");
    EXPECT_EQ(members[2]["DiagnosticDataType"], "CPER");
    EXPECT_FALSE(members[2].contains("Links"));

    EXPECT_EQ(members[3]["Id"], "100");
    EXPECT_FALSE(members[3].contains("CPER"));
    EXPECT_FALSE(members[3].contains("Links"));
}

TEST(AfterDBusEventLogEntryGet, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", true, ebadr(), {});
    expectNotFound(asyncResp->res, "EventLogEntry", "7");
}

TEST(AfterDBusEventLogEntryGet, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", true,
                              makeEc(boost::system::errc::timed_out), {});
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterDBusEventLogEntryGet, InvalidPropertiesReturnInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap props = makeEntryProps(7);
    props[0].second = std::string("7");
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", true, {}, props);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("CPER"));
}

TEST(AfterDBusEventLogEntryGet, NonCperEntry)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", false, {},
                              makeEntryProps(7));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Id"], "7");
    EXPECT_EQ(asyncResp->res.jsonValue["@odata.type"],
              "#LogEntry.v1_15_0.LogEntry");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("DiagnosticDataType"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("CPER"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Links"));
}

TEST(AfterDBusEventLogEntryGet, CperEntryWithoutRaw)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap props = makeEntryProps(7);
    for (auto& p :
         makeCperProps(cperSectionDbus, notificationGuid, sectionGuid))
    {
        props.emplace_back(std::move(p));
    }
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", false, {}, props);
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["DiagnosticDataType"], "CPERSection");
    EXPECT_FALSE(asyncResp->res.jsonValue["CPER"].contains("NotificationType"));
    EXPECT_EQ(asyncResp->res.jsonValue["CPER"]["SectionType"], sectionGuid);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Links"));
}

TEST(AfterDBusEventLogEntryGet, CperEntryWithRaw)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap props = makeEntryProps(7);
    for (auto& p : makeCperProps(cperDbus, notificationGuid, ""))
    {
        props.emplace_back(std::move(p));
    }
    props.emplace_back("Oem", OemDict{{"Info", R"({"a": "b"})"}});
    afterDBusEventLogEntryGet(asyncResp, collectionStr, memberId,
                              logEntryDescriptor, "7", true, {}, props);
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["DiagnosticDataType"], "CPER");
    EXPECT_EQ(asyncResp->res.jsonValue["CPER"]["Oem"]["Info"]["a"], "b");
    EXPECT_EQ(
        asyncResp->res.jsonValue["Links"]["RelatedLogEntries@odata.count"], 1);
    EXPECT_EQ(
        asyncResp->res.jsonValue["Links"]["RelatedLogEntries"][0]["@odata.id"],
        expectedRawCperUri(7));
}

TEST(AfterDBusEventLogEntryGetObject, MapperIoErrorReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGetObject(
        asyncResp, collectionStr, memberId, logEntryDescriptor, "9",
        "/xyz/openbmc_project/logging/entry/9",
        makeEc(boost::system::errc::io_error), {});
    expectNotFound(asyncResp->res, "EventLogEntry", "9");
}

TEST(AfterDBusEventLogEntryGetObject, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGetObject(
        asyncResp, collectionStr, memberId, logEntryDescriptor, "9",
        "/xyz/openbmc_project/logging/entry/9", ebadr(), {});
    expectNotFound(asyncResp->res, "EventLogEntry", "9");
}

TEST(AfterDBusEventLogEntryGetObject, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterDBusEventLogEntryGetObject(
        asyncResp, collectionStr, memberId, logEntryDescriptor, "9",
        "/xyz/openbmc_project/logging/entry/9",
        makeEc(boost::system::errc::timed_out), {});
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish::eventlog_utils
