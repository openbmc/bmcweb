#include "app.hpp"
#include "event_service_manager.hpp"
#include "redfish-core/lib/health.hpp"
#include "redfish-core/lib/log_services.hpp"
#include "redfish-core/lib/ut/sdbus_connection_mock.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using ::testing::Contains;
using ::testing::InvokeArgument;
using ::testing::Matcher;

TEST(LogServicesTest, LogServicesCreateDumpInvalidTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    redfish::createDump(asyncResp, req, "InvalidType", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateSystemDumpInvalidDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"DiagnosticDataType": "Invalid"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;
    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .Times(0);

    redfish::createDump(asyncResp, req, "System", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::actionParameterMissing(
                    "CollectDiagnosticData",
                    "DiagnosticDataType & OEMDiagnosticDataType")));
}

TEST(LogServicesTest, LogServicesCreateBMCDumpInvalidDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"DiagnosticDataType": "Invalid"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;
    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .Times(0);

    redfish::createDump(asyncResp, req, "BMC", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest,
     LogServicesCreateSystemDumpMissingOEMDiagnosticDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"DiagnosticDataType": "OEM"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .Times(0);

    redfish::createDump(asyncResp, req, "System", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::actionParameterMissing(
                    "CollectDiagnosticData",
                    "DiagnosticDataType & OEMDiagnosticDataType")));
}

TEST(LogServicesTest, LogServicesCreateSystemDumpMissingDiagnosticDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"OEMDiagnosticDataType": "System"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .Times(0);

    redfish::createDump(asyncResp, req, "System", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::actionParameterMissing(
                    "CollectDiagnosticData",
                    "DiagnosticDataType & OEMDiagnosticDataType")));
}

TEST(LogServicesTest, LogServicesCreateSystemDumpMakesAsyncCall)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body =
        R"({"DiagnosticDataType": "OEM", "OEMDiagnosticDataType": "System"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    // Set ec to 1 so that createDumpTaskCallback() does not get called. Testing
    // that function is outside the scope of this test.
    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()), 0));

    redfish::createDump(asyncResp, req, "System", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateBMCDumpMakesAsyncCall)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"DiagnosticDataType": "Manager"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    // Set ec to 1 so that createDumpTaskCallback() does not get called. Testing
    // that function is outside the scope of this test.
    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcDumpId_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()), 0));

    redfish::createDump(asyncResp, req, "BMC", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateFaultLogDumpUnsupported)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = R"({"DiagnosticDataType": "Manager"})";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    redfish::createDump(asyncResp, req, "FaultLog", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::method_not_allowed);
}

TEST(LogServicesTest, LogServicesGetDumpEntryCollectionInvalidTypeFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;
    redfish::getDumpEntryCollection(asyncResp, "InvalidType", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryCollectionBMCBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    redfish::getDumpEntryCollection(asyncResp, "BMC", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryCollectionSystemBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    redfish::getDumpEntryCollection(asyncResp, "System", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryCollectionFaultLogBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    redfish::getDumpEntryCollection(asyncResp, "FaultLog", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryCollectionFaultLogSuccess)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(0, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    redfish::getDumpEntryCollection(asyncResp, "FaultLog", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Members@odata.count"], 0);
}

TEST(LogServicesTest, LogServicesGetDumpEntryByIdInvalidTypeFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;
    redfish::getDumpEntryById(asyncResp, "", "InvalidType", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryByIdBMCBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    // Specify entryID as an empty string, causing an error
    redfish::getDumpEntryById(asyncResp, "", "BMC", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryByIdSystemBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    // Specify entryID as an empty string, causing an error
    redfish::getDumpEntryById(asyncResp, "", "System", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesGetDumpEntryByIdFaultLogBadECFails)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    MockSdbusConnection sdbusMock;

    EXPECT_CALL(sdbusMock,
                async_method_call(Matcher<handlerEcResp_t>(::testing::_),
                                  ::testing::_, ::testing::_, ::testing::_,
                                  ::testing::_))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(1, boost::system::system_category()),
            dbus::utility::ManagedObjectType{}));

    // Specify entryID as empty string, causing an error
    redfish::getDumpEntryById(asyncResp, "", "FaultLog", sdbusMock);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}
