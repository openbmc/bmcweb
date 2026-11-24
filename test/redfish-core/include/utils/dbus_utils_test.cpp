// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "http_response.hpp"
#include "utils/dbus_utils.hpp"

#include <asm-generic/errno.h>
#include <systemd/sd-bus-protocol.h>

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/test/sdbus_mock.hpp>

#include <cerrno>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace redfish::details
{
namespace
{

TEST(DbusUtils, AfterPropertySetSuccess)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec;
    sdbusplus::message_t msg;
    afterSetProperty(asyncResp, "MyRedfishProperty",
                     nlohmann::json("MyRedfishValue"), ec, msg);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::no_content);
}

TEST(DbusUtils, AfterActionPropertySetSuccess)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec;
    sdbusplus::message_t msg;
    afterSetPropertyAction(asyncResp, "MyRedfishProperty", "MyRedfishValue", ec,
                           msg);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                    "@Message.ExtendedInfo": [
                        {
                            "@odata.type": "#Message.v1_1_1.Message",
                            "Message": "The request completed successfully.",
                            "MessageArgs": [],
                            "MessageId": "Base.1.19.Success",
                            "MessageSeverity": "OK",
                            "Resolution": "None."
                        }
                    ]
                })"_json);
}

TEST(DbusUtils, AfterPropertySetInternalError)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::timed_out);
    sdbusplus::message_t msg;
    afterSetProperty(asyncResp, "MyRedfishProperty",
                     nlohmann::json("MyRedfishValue"), ec, msg);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue.size(), 1);
    using nlohmann::literals::operator""_json;

    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                    "error": {
                    "@Message.ExtendedInfo": [
                        {
                        "@odata.type": "#Message.v1_1_1.Message",
                        "Message": "The request failed due to an internal service error.  The service is still operational.",
                        "MessageArgs": [],
                        "MessageId": "Base.1.19.InternalError",
                        "MessageSeverity": "Critical",
                        "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                        }
                    ],
                    "code": "Base.1.19.InternalError",
                    "message": "The request failed due to an internal service error.  The service is still operational."
                    }
                })"_json);
}

} // namespace
} // namespace redfish::details

namespace redfish::dbus_utils
{
namespace
{

TEST(DbusUtils, HandleDbusErrorNoError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec;
    EXPECT_TRUE(handleDbusError(asyncResp, ec));
    EXPECT_TRUE(handleDbusError(asyncResp, ec, {}, {{EBADR, handleEBADR}}));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(DbusUtils, HandleDbusErrorEBADR)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec(EBADR, boost::system::generic_category());
    EXPECT_FALSE(handleDbusError(asyncResp, ec, {}, {{EBADR, handleEBADR}}));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(DbusUtils, HandleDbusErrorCallbackWithoutMessage)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    bool called = false;
    boost::system::error_code ec(EIO, boost::system::generic_category());
    EXPECT_FALSE(handleDbusError(
        asyncResp, ec, {},
        {{EIO, [&called, &ec](const boost::system::error_code& cbEc,
                              std::string_view errorName) {
              called = true;
              EXPECT_EQ(cbEc, ec);
              EXPECT_TRUE(errorName.empty());
          }}}));

    EXPECT_TRUE(called);
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(DbusUtils, HandleDbusErrorCallbackGetsErrorName)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    sdbusplus::SdBusMock sdbusMock;
    sd_bus_error error{.name = "xyz.openbmc_project.Common.Error.NotFound",
                       .message = "",
                       ._need_free = 0};
    EXPECT_CALL(sdbusMock, sd_bus_message_get_error(testing::_))
        .WillRepeatedly(testing::Return(&error));
    sdbusplus::message_t msg(nullptr, &sdbusMock, std::false_type{});

    std::string name;
    boost::system::error_code ec(EIO, boost::system::generic_category());
    EXPECT_FALSE(handleDbusError(
        asyncResp, ec, msg,
        {{EIO, [&name](const boost::system::error_code& /*cbEc*/,
                       std::string_view errorName) { name = errorName; }}}));

    EXPECT_EQ(name, "xyz.openbmc_project.Common.Error.NotFound");
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(DbusUtils, HandleDbusErrorEBADRInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec(EBADR, boost::system::generic_category());
    EXPECT_FALSE(handleDbusError(asyncResp, ec));

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(DbusUtils, HandleDbusErrorInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    boost::system::error_code ec(EIO, boost::system::generic_category());
    EXPECT_FALSE(handleDbusError(asyncResp, ec, {}, {{EBADR, handleEBADR}}));

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish::dbus_utils
