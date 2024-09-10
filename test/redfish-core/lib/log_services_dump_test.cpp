// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "log_services.hpp"

#include <boost/beast/http/status.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(LogServicesDumpServiceTest, LogServicesInvalidDumpServiceGetReturnsError)
{
    auto shareAsyncResp = std::make_shared<bmcweb::AsyncResp>();
    getDumpServiceInfo(shareAsyncResp, "Invalid");
    EXPECT_EQ(shareAsyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

} // namespace
} // namespace redfish
