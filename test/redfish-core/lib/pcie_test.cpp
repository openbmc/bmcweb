// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_response.hpp"
#include "pcie.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetPCIeDeviceUUID, EbadrOmitsUUID)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetPCIeDeviceUUID(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("UUID"));
}

TEST(AfterGetPCIeDeviceUUID, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetPCIeDeviceUUID(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetPCIeDeviceUUID, EmptyUUIDOmitsUUID)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetPCIeDeviceUUID(response, ec, "");

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("UUID"));
}

TEST(AfterGetPCIeDeviceUUID, SuccessSetsUUID)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetPCIeDeviceUUID(response, ec,
                           "11111111-2222-3333-4444-555566667777");

    EXPECT_EQ(response->res.jsonValue["UUID"],
              "11111111-2222-3333-4444-555566667777");
}

} // namespace
} // namespace redfish
