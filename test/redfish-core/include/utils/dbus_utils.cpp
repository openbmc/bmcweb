
#include "utils/dbus_utils.hpp"

#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>
#include <vector>

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

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue,
              R"({
                    "@Message.ExtendedInfo": [
                        {
                            "@odata.type": "#Message.v1_1_1.Message",
                            "Message": "The request completed successfully.",
                            "MessageArgs": [],
                            "MessageId": "Base.1.19.0.Success",
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
                        "MessageId": "Base.1.19.0.InternalError",
                        "MessageSeverity": "Critical",
                        "Resolution": "Resubmit the request.  If the problem persists, consider resetting the service."
                        }
                    ],
                    "code": "Base.1.19.0.InternalError",
                    "message": "The request failed due to an internal service error.  The service is still operational."
                    }
                })"_json);
}

} // namespace
} // namespace redfish::details
