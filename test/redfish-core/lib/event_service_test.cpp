// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "app.hpp"
#include "async_resp.hpp"
#include "event_service.hpp"
#include "http_request.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(HandleEventServiceSubscriptionsPost,
     SnmpWithDeliveryRetryPolicyReportsCorrectPropertyName)
{
    crow::App app;
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    crow::Request req = json_util::makeJsonPost(
        R"({"Destination": "snmp://192.168.1.1:162",
            "Protocol": "SNMPv2c",
            "DeliveryRetryPolicy": "RetryForever"})");

    handleEventServiceSubscriptionsPost(app, req, asyncResp);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    const nlohmann::json& info =
        asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0];
    EXPECT_EQ(info["MessageId"], "Base.1.19.PropertyValueConflict");
    EXPECT_EQ(info["MessageArgs"][0], "DeliveryRetryPolicy");
    EXPECT_EQ(info["MessageArgs"][1], "Protocol");
}

} // namespace
} // namespace redfish
