#include "redfish-core/lib/log_services.hpp"

#include <error_messages.hpp>

#include <string>
#include <variant>

#include "gmock/gmock.h"

using testing::_;
using testing::Contains;
using testing::InvokeArgument;
using testing::Matcher;
using testing::Not;

using handlerEc_t = std::function<void(boost::system::error_code ec)>&&;
using handlerEcObjPath_t =
    std::function<void(const boost::system::error_code ec,
                       const sdbusplus::message::object_path& objPath)>&&;
using variantMap_t = std::map<std::string, std::variant<std::string, uint64_t>>;

class MockSdbusConnection
{
  public:
    MOCK_METHOD(void, async_method_call,
                (handlerEc_t handler, const std::string& service,
                 const std::string& objpath, const std::string& interf,
                 const std::string& method, variantMap_t params),
                ());

    MOCK_METHOD(void, async_method_call,
                (handlerEcObjPath_t handler, const std::string& service,
                 const std::string& objpath, const std::string& interf,
                 const std::string& method, variantMap_t params),
                ());
};

TEST(LogServicesTest, LogServicesCreateDumpInvalidTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    redfish::createDump(asyncResp, req, "InvalidType",
                        crow::connections::systemBus);
    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateSystemDumpInvalidDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{\"DiagnosticDataType\": \"Invalid\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .Times(0);
    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
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
    req.body = "{\"DiagnosticDataType\": \"Invalid\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .Times(0);
    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
        .Times(0);

    redfish::createDump(asyncResp, req, "BMC", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateFaultLogDumpInvalidDataTypeFails)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{\"DiagnosticDataType\": \"Invalid\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .Times(0);
    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
        .Times(0);

    redfish::createDump(asyncResp, req, "FaultLog", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Contains(redfish::messages::internalError()));
}

TEST(LogServicesTest, LogServicesCreateSystemDumpSucceeds)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{\"DiagnosticDataType\": \"OEM\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .Times(0);
    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
        .Times(0);

    redfish::createDump(asyncResp, req, "System", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"],
                Not(Contains(redfish::messages::internalError())));
}

TEST(LogServicesTest, LogServicesCreateBMCDumpSucceeds)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{\"DiagnosticDataType\": \"Manager\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .Times(0);

    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(0, boost::system::system_category()),
            sdbusplus::message::object_path("path")));

    redfish::createDump(asyncResp, req, "BMC", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["@Message.ExtendedInfo"],
                Contains(redfish::messages::success()));
}

TEST(LogServicesTest, LogServicesCreateFaultLogDumpSucceeds)
{
    std::error_code ec;
    crow::Request req({}, ec);
    req.body = "{\"DiagnosticDataType\": \"Manager\"}";

    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    asyncResp->res.setCompleteRequestHandler(nullptr);

    auto sdbusMock = std::make_shared<MockSdbusConnection>();
    EXPECT_CALL(*sdbusMock,
                async_method_call(Matcher<handlerEc_t>(_), _, _, _, _, _))
        .WillOnce(InvokeArgument<0>(
            boost::system::error_code(0, boost::system::system_category())));
    EXPECT_CALL(*sdbusMock, async_method_call(Matcher<handlerEcObjPath_t>(_), _,
                                              _, _, _, _))
        .Times(0);

    redfish::createDump(asyncResp, req, "FaultLog", sdbusMock);

    EXPECT_THAT(asyncResp->res.jsonValue["@Message.ExtendedInfo"],
                Contains(redfish::messages::success()));
}

TEST(LogServicesTestFailure, BasicFailure)
{
    EXPECT_TRUE(false);
}
