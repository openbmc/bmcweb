// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "led.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

nlohmann::json internalErrorJson()
{
    return R"({
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
    })"_json;
}

nlohmann::json locationIndicatorUnknownJson()
{
    return R"({
        "error": {
            "@Message.ExtendedInfo": [
                {
                    "@odata.type": "#Message.v1_1_1.Message",
                    "Message": "The property LocationIndicatorActive is not in the list of valid properties for the resource.",
                    "MessageArgs": [
                        "LocationIndicatorActive"
                    ],
                    "MessageId": "Base.1.19.PropertyUnknown",
                    "MessageSeverity": "Warning",
                    "Resolution": "Remove the unknown property from the request body and resubmit the request if the operation failed."
                }
            ],
            "code": "Base.1.19.PropertyUnknown",
            "message": "The property LocationIndicatorActive is not in the list of valid properties for the resource."
        }
    })"_json;
}

boost::system::error_code invalidArgument()
{
    return boost::system::errc::make_error_code(
        boost::system::errc::invalid_argument);
}

boost::system::error_code timedOut()
{
    return boost::system::errc::make_error_code(boost::system::errc::timed_out);
}

boost::system::error_code badRequestDescriptor()
{
    return boost::system::linux_error::bad_request_descriptor;
}

// Records what handleLedGroupSubtree() forwards to its callback.  The string
// members start out non-empty so that an assertion on "" proves the callback
// really overwrote them.
struct LedGroupRecorder
{
    bool called = false;
    boost::system::error_code ec;
    std::string ledGroupPath = "unset";
    std::string service = "unset";

    std::function<void(const boost::system::error_code&, const std::string&,
                       const std::string&)>
        callback()
    {
        return [this](const boost::system::error_code& callbackEc,
                      const std::string& callbackPath,
                      const std::string& callbackService) {
            called = true;
            ec = callbackEc;
            ledGroupPath = callbackPath;
            service = callbackService;
        };
    }
};

TEST(HandleLedGroupSubtree, ErrorForwardsErrorWithEmptyPathAndService)
{
    LedGroupRecorder recorder;

    handleLedGroupSubtree("/xyz/openbmc_project/inventory/system/chassis",
                          timedOut(), {}, recorder.callback());

    EXPECT_TRUE(recorder.called);
    EXPECT_EQ(recorder.ec, timedOut());
    EXPECT_EQ(recorder.ledGroupPath, "");
    EXPECT_EQ(recorder.service, "");
}

TEST(HandleLedGroupSubtree, EmptySubtreeForwardsEmptyPathAndService)
{
    LedGroupRecorder recorder;
    boost::system::error_code ec;

    handleLedGroupSubtree("/xyz/openbmc_project/inventory/system/chassis", ec,
                          {}, recorder.callback());

    EXPECT_TRUE(recorder.called);
    EXPECT_FALSE(recorder.ec);
    EXPECT_EQ(recorder.ledGroupPath, "");
    EXPECT_EQ(recorder.service, "");
}

TEST(HandleLedGroupSubtree, MultipleLedGroupsForwardEmptyPathAndService)
{
    LedGroupRecorder recorder;
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/led/groups/chassis_identify",
         {{"xyz.openbmc_project.LED.GroupManager",
           {"xyz.openbmc_project.Led.Group"}}}},
        {"/xyz/openbmc_project/led/groups/enclosure_identify",
         {{"xyz.openbmc_project.LED.GroupManager",
           {"xyz.openbmc_project.Led.Group"}}}}};

    handleLedGroupSubtree("/xyz/openbmc_project/inventory/system/chassis", ec,
                          subtree, recorder.callback());

    EXPECT_TRUE(recorder.called);
    EXPECT_FALSE(recorder.ec);
    EXPECT_EQ(recorder.ledGroupPath, "");
    EXPECT_EQ(recorder.service, "");
}

TEST(HandleLedGroupSubtree, SingleLedGroupForwardsPathAndService)
{
    LedGroupRecorder recorder;
    boost::system::error_code ec;
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/led/groups/chassis_identify",
         {{"xyz.openbmc_project.LED.GroupManager",
           {"xyz.openbmc_project.Led.Group"}}}}};

    handleLedGroupSubtree("/xyz/openbmc_project/inventory/system/chassis", ec,
                          subtree, recorder.callback());

    EXPECT_TRUE(recorder.called);
    EXPECT_FALSE(recorder.ec);
    EXPECT_EQ(recorder.ledGroupPath,
              "/xyz/openbmc_project/led/groups/chassis_identify");
    EXPECT_EQ(recorder.service, "xyz.openbmc_project.LED.GroupManager");
}

TEST(AfterGetLocationIndicatorEnclosure, InvalidArgumentSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLocationIndicatorEnclosure(response, invalidArgument(), true);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
}

TEST(AfterGetLocationIndicatorEnclosure, OtherErrorLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLocationIndicatorEnclosure(response, timedOut(), true);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
}

TEST(AfterGetLocationIndicatorEnclosure, LedOnSetsLocationIndicatorActiveTrue)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetLocationIndicatorEnclosure(response, ec, true);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue,
              R"({"LocationIndicatorActive": true})"_json);
}

TEST(AfterGetLocationIndicatorEnclosure, LedOffSetsLocationIndicatorActiveFalse)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetLocationIndicatorEnclosure(response, ec, false);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue,
              R"({"LocationIndicatorActive": false})"_json);
}

TEST(AfterGetLocationIndicatorBlink, InvalidArgumentSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    afterGetLocationIndicatorBlink(response, invalidArgument(), false);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
}

TEST(AfterGetLocationIndicatorBlink, BlinkingSetsLocationIndicatorActiveTrue)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetLocationIndicatorBlink(response, ec, true);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue,
              R"({"LocationIndicatorActive": true})"_json);
}

TEST(AfterGetLedState, ErrorSetsInternalErrorAndSkipsCallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;

    afterGetLedState(
        response, [&asserted](bool value) { asserted = value; }, timedOut(),
        true);

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
    EXPECT_FALSE(asserted.has_value());
}

TEST(AfterGetLedState, EbadrSkipsCallbackAndLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;

    afterGetLedState(
        response, [&asserted](bool value) { asserted = value; },
        badRequestDescriptor(), true);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
    EXPECT_FALSE(asserted.has_value());
}

TEST(AfterGetLedState, SuccessForwardsAssertedToCallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;
    boost::system::error_code ec;

    afterGetLedState(
        response, [&asserted](bool value) { asserted = value; }, ec, true);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
    EXPECT_EQ(asserted, std::optional<bool>(true));

    asserted.reset();
    afterGetLedState(
        response, [&asserted](bool value) { asserted = value; }, ec, false);

    EXPECT_EQ(asserted, std::optional<bool>(false));
}

TEST(GetLedState, ErrorSetsInternalErrorAndSkipsCallback)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;

    getLedState(
        response, [&asserted](bool value) { asserted = value; }, timedOut(),
        "/xyz/openbmc_project/led/groups/chassis_identify",
        "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
    EXPECT_FALSE(asserted.has_value());
}

TEST(GetLedState, EbadrSkipsCallbackAndLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;

    getLedState(
        response, [&asserted](bool value) { asserted = value; },
        badRequestDescriptor(),
        "/xyz/openbmc_project/led/groups/chassis_identify",
        "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
    EXPECT_FALSE(asserted.has_value());
}

TEST(GetLedState, EmptyLedGroupPathSkipsCallbackAndLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;
    boost::system::error_code ec;

    getLedState(
        response, [&asserted](bool value) { asserted = value; }, ec, "",
        "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
    EXPECT_FALSE(asserted.has_value());
}

TEST(GetLedState, EmptyServiceSkipsCallbackAndLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    std::optional<bool> asserted;
    boost::system::error_code ec;

    getLedState(
        response, [&asserted](bool value) { asserted = value; }, ec,
        "/xyz/openbmc_project/led/groups/chassis_identify", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
    EXPECT_FALSE(asserted.has_value());
}

TEST(SetLedState, EbadrSetsPropertyUnknown)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    setLedState(response, true, badRequestDescriptor(),
                "/xyz/openbmc_project/led/groups/chassis_identify",
                "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(response->res.jsonValue, locationIndicatorUnknownJson());
}

TEST(SetLedState, OtherErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    setLedState(response, true, timedOut(),
                "/xyz/openbmc_project/led/groups/chassis_identify",
                "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
}

TEST(SetLedState, EmptyLedGroupPathSetsPropertyUnknown)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    setLedState(response, true, ec, "", "xyz.openbmc_project.LED.GroupManager");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(response->res.jsonValue, locationIndicatorUnknownJson());
}

TEST(SetLedState, EmptyServiceSetsPropertyUnknown)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    setLedState(response, true, ec,
                "/xyz/openbmc_project/led/groups/chassis_identify", "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(response->res.jsonValue, locationIndicatorUnknownJson());
}

TEST(SetIndicatorLedState, UnknownStateSetsPropertyValueNotInList)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();

    setIndicatorLedState(response, "Rainbow");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::bad_request);
    EXPECT_EQ(response->res.jsonValue, R"({
        "error": {
            "@Message.ExtendedInfo": [
                {
                    "@odata.type": "#Message.v1_1_1.Message",
                    "Message": "The value '\"Rainbow\"' for the property IndicatorLED is not in the list of acceptable values.",
                    "MessageArgs": [
                        "\"Rainbow\"",
                        "IndicatorLED"
                    ],
                    "MessageId": "Base.1.19.PropertyValueNotInList",
                    "MessageSeverity": "Warning",
                    "Resolution": "Choose a value from the enumeration list that the implementation can support and resubmit the request if the operation failed."
                }
            ],
            "code": "Base.1.19.PropertyValueNotInList",
            "message": "The value '\"Rainbow\"' for the property IndicatorLED is not in the list of acceptable values."
        }
    })"_json);
}

TEST(AfterSetLocationIndicatorBlink, SuccessLeavesResponseUntouched)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterSetLocationIndicatorBlink(response, true, ec);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(response->res.jsonValue.is_null());
}

} // namespace
} // namespace redfish
