// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "duplicatable_file_handle.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "log_services_rawcper.hpp"
#include "routing.hpp"
#include "routing/baserule.hpp"

#include <asm-generic/errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

using nlohmann::literals::operator""_json;

constexpr const char* notificationGuid = "E8F56FFE-919C-4CC5-BA88-65ABE14913BB";
constexpr const char* sectionGuid = "9876CCAD-47B4-4BDB-B65E-16F193C4F3DB";
constexpr const char* rawIface = "xyz.openbmc_project.Logging.CPER.Raw";
constexpr const char* cperDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPER";
constexpr const char* cperSectionDbus =
    "xyz.openbmc_project.Logging.CPER.Types.ContentType.CPERSection";
constexpr std::string_view badParentId = "bad";

// Location-dependent values, mirroring BMCWEB_REDFISH_EVENTLOG_LOCATION.
struct Parent
{
    std::string_view collection;
    std::string_view memberId;
    std::string_view type;
    std::string_view otherCollection;
};

constexpr Parent parent()
{
    if constexpr (BMCWEB_REDFISH_EVENTLOG_LOCATION == "managers")
    {
        return {"Managers", BMCWEB_REDFISH_MANAGER_URI_NAME, "Manager",
                "Systems"};
    }
    else
    {
        return {"Systems", BMCWEB_REDFISH_SYSTEM_URI_NAME, "ComputerSystem",
                "Managers"};
    }
}

std::string serviceUri()
{
    return std::format("/redfish/v1/{}/{}/LogServices/CPER",
                       parent().collection, parent().memberId);
}

std::string entryUri(std::string_view encodedId)
{
    return std::format("{}/Entries/{}", serviceUri(), encodedId);
}

boost::system::error_code ebadr()
{
    return {EBADR, boost::system::system_category()};
}

boost::system::error_code makeEc(boost::system::errc::errc_t e)
{
    return boost::system::errc::make_error_code(e);
}

dbus::utility::DBusPropertiesMap makeProps(const std::string& type,
                                           const std::string& notification,
                                           const std::string& section)
{
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", type);
    props.emplace_back("NotificationType", notification);
    props.emplace_back("SectionType", section);
    return props;
}

void expectNotFound(crow::Response& res, std::string_view type,
                    std::string_view id)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::not_found);
    EXPECT_EQ(res.jsonValue["error"]["@Message.ExtendedInfo"][0]["MessageArgs"],
              nlohmann::json::array({type, id}));
}

void expectInternalError(crow::Response& res)
{
    EXPECT_EQ(res.result(), boost::beast::http::status::internal_server_error);
    EXPECT_NE(res.getHeaderValue(boost::beast::http::field::content_type),
              "application/octet-stream");
}

crow::Request makeRequest()
{
    std::error_code ec;
    return crow::Request{{boost::beast::http::verb::get, "/whatever", 11}, ec};
}

TEST(FillRawCperLogEntryFromPropertyMap, FullCperRecord)
{
    nlohmann::json j = nlohmann::json::object();
    fillRawCperLogEntryFromPropertyMap(
        makeProps(cperDbus, notificationGuid, ""), "5", j);

    nlohmann::json expected = R"({
        "@odata.type": "#LogEntry.v1_15_0.LogEntry",
        "Id": "5",
        "Name": "Raw CPER Log Entry",
        "EntryType": "Event",
        "DiagnosticDataType": "CPER",
        "CPER": {"NotificationType": "E8F56FFE-919C-4CC5-BA88-65ABE14913BB"}
    })"_json;
    expected["@odata.id"] = entryUri("5");
    expected["AdditionalDataURI"] = entryUri("5") + "/attachment";
    EXPECT_EQ(j, expected);
}

TEST(FillRawCperLogEntryFromPropertyMap, FullCperSectionRecord)
{
    nlohmann::json j = nlohmann::json::object();
    fillRawCperLogEntryFromPropertyMap(
        makeProps(cperSectionDbus, notificationGuid, sectionGuid), "6", j);

    nlohmann::json expected = R"({
        "@odata.type": "#LogEntry.v1_15_0.LogEntry",
        "Id": "6",
        "Name": "Raw CPER Log Entry",
        "EntryType": "Event",
        "DiagnosticDataType": "CPERSection",
        "CPER": {"SectionType": "9876CCAD-47B4-4BDB-B65E-16F193C4F3DB"}
    })"_json;
    expected["@odata.id"] = entryUri("6");
    expected["AdditionalDataURI"] = entryUri("6") + "/attachment";
    EXPECT_EQ(j, expected);
}

TEST(FillRawCperLogEntryFromPropertyMap, EmptyGuidsGiveEmptyCperObject)
{
    for (const char* type : {cperDbus, cperSectionDbus})
    {
        nlohmann::json j = nlohmann::json::object();
        fillRawCperLogEntryFromPropertyMap(makeProps(type, "", ""), "1", j);
        EXPECT_EQ(j["CPER"], nlohmann::json::object()) << "type: " << type;
    }
}

TEST(FillRawCperLogEntryFromPropertyMap, UnknownDiagnosticDataTypeIsOmitted)
{
    nlohmann::json j = nlohmann::json::object();
    fillRawCperLogEntryFromPropertyMap(
        makeProps("xyz.openbmc_project.Logging.CPER.Types.ContentType.OEM",
                  notificationGuid, sectionGuid),
        "1", j);
    EXPECT_FALSE(j.contains("DiagnosticDataType"));
    // The rest of the entry is still rendered, but without either GUID.
    EXPECT_EQ(j["Id"], "1");
    EXPECT_EQ(j["CPER"], nlohmann::json::object());
    EXPECT_EQ(j["AdditionalDataURI"], entryUri("1") + "/attachment");
}

TEST(FillRawCperLogEntryFromPropertyMap, WrongTypedDiagnosticDataTypeOmitted)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props;
    props.emplace_back("DiagnosticDataType", uint32_t{1});
    fillRawCperLogEntryFromPropertyMap(props, "1", j);
    EXPECT_FALSE(j.contains("DiagnosticDataType"));
    EXPECT_EQ(j["Id"], "1");
}

TEST(FillRawCperLogEntryFromPropertyMap, NoPropertiesStillRendersEntry)
{
    nlohmann::json j = nlohmann::json::object();
    fillRawCperLogEntryFromPropertyMap({}, "1", j);
    EXPECT_EQ(j["@odata.id"], entryUri("1"));
    EXPECT_EQ(j["Id"], "1");
    EXPECT_FALSE(j.contains("DiagnosticDataType"));
    EXPECT_EQ(j["CPER"], nlohmann::json::object());
    EXPECT_EQ(j["AdditionalDataURI"], entryUri("1") + "/attachment");
}

// The raw entry carries no Oem and does not link back to the EventLog.
TEST(FillRawCperLogEntryFromPropertyMap, NoOemAndNoBackLink)
{
    nlohmann::json j = nlohmann::json::object();
    dbus::utility::DBusPropertiesMap props =
        makeProps(cperSectionDbus, notificationGuid, sectionGuid);
    props.emplace_back(
        "Oem", std::vector<std::pair<std::string, std::string>>{{"k", "v"}});
    fillRawCperLogEntryFromPropertyMap(props, "1", j);
    EXPECT_FALSE(j["CPER"].contains("Oem"));
    EXPECT_FALSE(j.contains("Links"));
    EXPECT_FALSE(j.contains("Message"));
}

TEST(FillRawCperLogEntryFromPropertyMap, EntryIdIsPercentEncodedInUris)
{
    nlohmann::json j = nlohmann::json::object();
    fillRawCperLogEntryFromPropertyMap({}, "a b", j);
    EXPECT_EQ(j["Id"], "a b");
    EXPECT_EQ(j["@odata.id"], entryUri("a%20b"));
    EXPECT_EQ(j["AdditionalDataURI"], entryUri("a%20b") + "/attachment");
}

TEST(AfterRawCperEntryGetObject, MapperIoErrorReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryGetObject(asyncResp, "3",
                               makeProps(cperDbus, notificationGuid, ""),
                               makeEc(boost::system::errc::io_error), {});
    expectNotFound(asyncResp->res, "LogEntry", "3");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("CPER"));
}

TEST(AfterRawCperEntryGetObject, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryGetObject(asyncResp, "3", {}, ebadr(), {});
    expectNotFound(asyncResp->res, "LogEntry", "3");
}

TEST(AfterRawCperEntryGetObject, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryGetObject(asyncResp, "3", {},
                               makeEc(boost::system::errc::timed_out), {});
    expectInternalError(asyncResp->res);
}

// Logging.Entry exists but has no CPER.Raw interface.
TEST(AfterRawCperEntryGetObject, NoRawInterfaceReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryGetObject(
        asyncResp, "3", makeProps(cperDbus, notificationGuid, ""), {}, {});
    expectNotFound(asyncResp->res, "LogEntry", "3");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("CPER"));
}

TEST(AfterRawCperEntryGetObject, RawInterfacePresentRendersEntry)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.Logging", {rawIface}}};
    afterRawCperEntryGetObject(
        asyncResp, "3",
        makeProps(cperSectionDbus, notificationGuid, sectionGuid), {}, object);
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Id"], "3");
    EXPECT_EQ(asyncResp->res.jsonValue["@odata.id"], entryUri("3"));
    EXPECT_EQ(asyncResp->res.jsonValue["DiagnosticDataType"], "CPERSection");
    EXPECT_EQ(asyncResp->res.jsonValue["CPER"]["SectionType"], sectionGuid);
}

TEST(AfterRawCperEntryGet, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryGet(asyncResp, "11", ebadr(), {});
    expectNotFound(asyncResp->res, "LogEntry", "11");
}

TEST(AfterRawCperEntryGet, OtherErrorsReturnInternalError)
{
    for (boost::system::errc::errc_t e :
         {boost::system::errc::timed_out, boost::system::errc::io_error,
          boost::system::errc::permission_denied})
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        afterRawCperEntryGet(asyncResp, "11", makeEc(e), {});
        EXPECT_EQ(asyncResp->res.result(),
                  boost::beast::http::status::internal_server_error)
            << "errc: " << static_cast<int>(e);
    }
}

int openRw(const std::string& path)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    return open(path.c_str(), O_RDWR | O_CLOEXEC);
}

TEST(AfterRawCperEntryAttachment, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryAttachment(asyncResp, "2", ebadr(),
                                sdbusplus::message::unix_fd{-1});
    expectNotFound(asyncResp->res, "LogEntry", "2");
}

TEST(AfterRawCperEntryAttachment, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryAttachment(asyncResp, "2",
                                makeEc(boost::system::errc::timed_out),
                                sdbusplus::message::unix_fd{-1});
    expectInternalError(asyncResp->res);
}

TEST(AfterRawCperEntryAttachment, InvalidFdReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryAttachment(asyncResp, "2", {},
                                sdbusplus::message::unix_fd{-1});
    expectInternalError(asyncResp->res);
}

TEST(AfterRawCperEntryAttachment, EmptyFileReturnsInternalError)
{
    DuplicatableFileHandle tmp("");
    int fd = openRw(tmp.filePath);
    ASSERT_GE(fd, 0);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryAttachment(asyncResp, "2", {},
                                sdbusplus::message::unix_fd{fd});
    expectInternalError(asyncResp->res);
    close(fd);
}

TEST(AfterRawCperEntryAttachment, ValidFileIsServedAsOctetStream)
{
    // Binary payload, including NUL and high bytes.
    const std::string data("CPER\x00\x01\xff\x7f", 8);
    DuplicatableFileHandle tmp(data);
    int fd = openRw(tmp.filePath);
    ASSERT_GE(fd, 0);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    afterRawCperEntryAttachment(asyncResp, "2", {},
                                sdbusplus::message::unix_fd{fd});
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(
        asyncResp->res.getHeaderValue(boost::beast::http::field::content_type),
        "application/octet-stream");
    EXPECT_TRUE(asyncResp->res.response.body().file().is_open());
    EXPECT_EQ(asyncResp->res.response.body().payloadSize(),
              std::optional<size_t>(data.size()));
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());

    // The caller's fd was dup'd, not taken; it must still be open.
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg)
    EXPECT_NE(fcntl(fd, F_GETFD), -1);
    close(fd);
}

TEST(HandleRawCperLogServiceGet, WrongParentReturnsNotFound)
{
    crow::App app;
    crow::Request req = makeRequest();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    handleRawCperLogServiceGet(app, req, asyncResp, std::string(badParentId));
    expectNotFound(asyncResp->res, parent().type, badParentId);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.type"));
}

TEST(HandleRawCperLogServiceGet, StaticAttributesAreExpected)
{
    crow::App app;
    crow::Request req = makeRequest();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    handleRawCperLogServiceGet(app, req, asyncResp,
                               std::string(parent().memberId));

    nlohmann::json& j = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(j["@odata.type"], "#LogService.v1_2_0.LogService");
    EXPECT_EQ(j["@odata.id"], serviceUri());
    EXPECT_EQ(j["Id"], "CPER");
    EXPECT_EQ(j["Name"], "Raw CPER Log Service");
    EXPECT_EQ(
        j["Description"],
        "Raw CPER Log Service holding the raw CPER binary for CPER events");
    EXPECT_EQ(j["Entries"],
              nlohmann::json({{"@odata.id", serviceUri() + "/Entries"}}));
    const std::string* dateTime = j["DateTime"].get_ptr<const std::string*>();
    ASSERT_NE(dateTime, nullptr);
    EXPECT_FALSE(dateTime->empty());
    const std::string* offset =
        j["DateTimeLocalOffset"].get_ptr<const std::string*>();
    ASSERT_NE(offset, nullptr);
    EXPECT_FALSE(offset->empty());
    // Read-only service: no ClearLog action and no inline Members.
    EXPECT_FALSE(j.contains("Actions"));
    EXPECT_FALSE(j.contains("Members"));
    EXPECT_FALSE(j.contains("error"));
}

TEST(HandleRawCperEntryCollection, WrongParentReturnsNotFound)
{
    crow::App app;
    crow::Request req = makeRequest();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    handleRawCperEntryCollection(app, req, asyncResp, std::string(badParentId));
    expectNotFound(asyncResp->res, parent().type, badParentId);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Members"));
}

TEST(HandleRawCperEntryGet, WrongParentReturnsNotFound)
{
    crow::App app;
    crow::Request req = makeRequest();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    handleRawCperEntryGet(app, req, asyncResp, std::string(badParentId), "1");
    expectNotFound(asyncResp->res, parent().type, badParentId);
}

TEST(HandleRawCperEntryAttachment, WrongParentReturnsNotFound)
{
    crow::App app;
    crow::Request req = makeRequest();
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    handleRawCperEntryAttachment(app, req, asyncResp, std::string(badParentId),
                                 "1");
    expectNotFound(asyncResp->res, parent().type, badParentId);
    EXPECT_NE(
        asyncResp->res.getHeaderValue(boost::beast::http::field::content_type),
        "application/octet-stream");
}

// getRoutes() does not descend into <str> segments, so resolve concrete URLs.
crow::Router::FindRoute findGetRoute(const crow::App& app, std::string_view url)
{
    std::error_code ec;
    crow::Request req{{boost::beast::http::verb::get, url, 11}, ec};
    return app.router.findRoute(req).route;
}

TEST(RequestRoutesRawCper, RegistersRoutesUnderConfiguredParent)
{
    crow::App app;
    requestRoutesRawCper(app);
    app.validate();

    const std::string base = std::format(
        "/redfish/v1/{}/<str>/LogServices/CPER/", parent().collection);
    const std::string member(parent().memberId);
    struct Case
    {
        std::string url;
        std::string rule;
        std::vector<std::string> params;
    };
    const std::array<Case, 4> cases = {{
        {serviceUri(), base, {member}},
        {serviceUri() + "/Entries", base + "Entries/", {member}},
        {entryUri("7"), base + "Entries/<str>/", {member, "7"}},
        {entryUri("7") + "/attachment",
         base + "Entries/<str>/attachment/",
         {member, "7"}},
    }};
    for (const Case& c : cases)
    {
        crow::Router::FindRoute route = findGetRoute(app, c.url);
        ASSERT_NE(route.rule, nullptr) << "url: " << c.url;
        EXPECT_EQ(route.rule->rule, c.rule);
        EXPECT_EQ(route.params, c.params) << "url: " << c.url;
    }
}

TEST(RequestRoutesRawCper, DoesNotRegisterUnderOtherParent)
{
    crow::App app;
    requestRoutesRawCper(app);
    app.validate();

    std::string_view other = parent().otherCollection;
    for (const std::string& url :
         {std::format("/redfish/v1/{}/{}/LogServices/CPER", other,
                      parent().memberId),
          std::format("/redfish/v1/{}/{}/LogServices/CPER/Entries/1", other,
                      parent().memberId)})
    {
        EXPECT_EQ(findGetRoute(app, url).rule, nullptr) << "url: " << url;
    }
}

} // namespace
} // namespace redfish
