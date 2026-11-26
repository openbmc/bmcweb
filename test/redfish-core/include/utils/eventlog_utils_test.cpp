// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_response.hpp"
#include "utils/eventlog_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>

#include <memory>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

boost::system::error_code ioError()
{
    return boost::system::errc::make_error_code(boost::system::errc::io_error);
}

TEST(AfterDBusEventLogEntryDelete, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    eventlog_utils::afterDBusEventLogEntryDelete(asyncResp, "1", ioError());

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

TEST(AfterDBusLogServiceActionsClear, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    eventlog_utils::afterDBusLogServiceActionsClear(asyncResp, ioError());

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_EQ(asyncResp->res.jsonValue["error"]["@Message.ExtendedInfo"][0],
              messages::internalError());
}

} // namespace
} // namespace redfish
