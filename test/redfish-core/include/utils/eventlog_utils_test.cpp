// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "http_response.hpp"
#include "utils/eventlog_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish::eventlog_utils
{
namespace
{

// Returns the Redfish MessageId stored in error.code, or "" if there is none.
std::string errorCode(const nlohmann::json& json)
{
    auto error = json.find("error");
    if (error == json.end())
    {
        return "";
    }
    auto code = error->find("code");
    if (code == error->end() || !code->is_string())
    {
        return "";
    }
    return code->get<std::string>();
}

TEST(AfterDBusEventLogEntryDelete, SuccessReturnsOk)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterDBusEventLogEntryDelete(asyncResp, "1", {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(errorCode(asyncResp->res.jsonValue), "");
}

TEST(AfterDBusEventLogEntryDelete, EbadrReturnsResourceNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterDBusEventLogEntryDelete(
        asyncResp, "1", boost::system::linux_error::bad_request_descriptor);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_TRUE(
        errorCode(asyncResp->res.jsonValue).ends_with("ResourceNotFound"));
}

TEST(AfterDBusEventLogEntryDelete, OtherErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterDBusEventLogEntryDelete(
        asyncResp, "1",
        boost::system::errc::make_error_code(boost::system::errc::io_error));

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(errorCode(asyncResp->res.jsonValue).ends_with("InternalError"));
}

TEST(AfterDBusLogServiceActionsClear, SuccessReturnsOk)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterDBusLogServiceActionsClear(asyncResp, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(errorCode(asyncResp->res.jsonValue), "");
}

TEST(AfterDBusLogServiceActionsClear, ErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterDBusLogServiceActionsClear(
        asyncResp,
        boost::system::errc::make_error_code(boost::system::errc::io_error));

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(errorCode(asyncResp->res.jsonValue).ends_with("InternalError"));
}

} // namespace
} // namespace redfish::eventlog_utils
