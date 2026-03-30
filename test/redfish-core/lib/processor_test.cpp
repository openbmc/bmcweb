// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "http_response.hpp"
#include "processor.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(AfterGetProcessorLocationCode, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetProcessorLocationCode(response, ec, "");

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetProcessorLocationCode, SuccessSetsServiceLabel)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetProcessorLocationCode(response, ec, "U3-P0");

    EXPECT_EQ(
        response->res.jsonValue["Location"]["PartLocation"]["ServiceLabel"],
        "U3-P0");
}

} // namespace
} // namespace redfish
