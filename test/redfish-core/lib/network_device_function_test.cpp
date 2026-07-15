// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_response.hpp"
#include "network_device_function.hpp"

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

TEST(AfterGetNDFInfiniBand, EbadrOmitsInfiniBand)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetNDFInfiniBand(response, ec, {});

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("InfiniBand"));
}

TEST(AfterGetNDFInfiniBand, ErrorSetsInternalError)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetNDFInfiniBand(response, ec, {});

    EXPECT_EQ(response->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetNDFInfiniBand, EmptyGuidsOmitInfiniBand)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties = {
        {"PermanentNodeGUID", std::string()},
        {"PermanentPortGUID", std::string()}};

    afterGetNDFInfiniBand(response, ec, properties);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(response->res.jsonValue.contains("InfiniBand"));
}

TEST(AfterGetNDFInfiniBand, SuccessSetsGuids)
{
    auto response = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    std::string nodeGuid = "F0FB:7F03:00A4:2B64";
    std::string portGuid = "F0FB:7F03:00A4:2B65";
    dbus::utility::DBusPropertiesMap properties = {
        {"PermanentNodeGUID", nodeGuid}, {"PermanentPortGUID", portGuid}};

    afterGetNDFInfiniBand(response, ec, properties);

    EXPECT_EQ(response->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(response->res.jsonValue["InfiniBand"]["PermanentNodeGUID"],
              nodeGuid);
    EXPECT_EQ(response->res.jsonValue["InfiniBand"]["PermanentPortGUID"],
              portGuid);
}

} // namespace
} // namespace redfish
