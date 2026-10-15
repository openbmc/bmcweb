// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "cable.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/resource.hpp"
#include "http_response.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <limits>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

TEST(FillCableProperties, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    fillCableProperties(asyncResp, boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(FillCableProperties, WrongPropertyTypeSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"Length", std::string("long")}};

    fillCableProperties(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("LengthMeters"));
}

TEST(FillCableProperties, EmptyPropertiesLeaveResponseEmpty)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    fillCableProperties(asyncResp, {}, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(asyncResp->res.jsonValue.empty());
}

TEST(FillCableProperties, PropertiesMapped)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"CableTypeDescription", std::string("Optical")}, {"Length", 1.5}};

    fillCableProperties(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["CableType"], "Optical");
    EXPECT_EQ(asyncResp->res.jsonValue["LengthMeters"], 1.5);
}

TEST(FillCableProperties, NanLengthOmitsLengthMeters)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"Length", std::numeric_limits<double>::quiet_NaN()}};

    fillCableProperties(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("LengthMeters"));
}

TEST(FillCableProperties, InfiniteLengthSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"Length", std::numeric_limits<double>::infinity()}};

    fillCableProperties(asyncResp, {}, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("LengthMeters"));
}

TEST(AfterHandleCableGet, EbadrReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterHandleCableGet(asyncResp, "cable0",
                        boost::system::linux_error::bad_request_descriptor, {});

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleCableGet, OtherErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    afterHandleCableGet(asyncResp, "cable0",
                        boost::asio::error::invalid_argument, {});

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterHandleCableGet, NoMatchingCableReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/system/cable1", {}}};

    afterHandleCableGet(asyncResp, "cable0", {}, subtree);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("@odata.id"));
}

TEST(AfterHandleCableGet, MatchingCablePopulatesResource)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::MapperGetSubTreeResponse subtree = {
        {"/xyz/openbmc_project/inventory/system/cable1", {}},
        {"/xyz/openbmc_project/inventory/system/cable0", {}}};

    afterHandleCableGet(asyncResp, "cable0", {}, subtree);

    const nlohmann::json& json = asyncResp->res.jsonValue;
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(json["@odata.type"], "#Cable.v1_0_0.Cable");
    EXPECT_EQ(json["@odata.id"], "/redfish/v1/Cables/cable0");
    EXPECT_EQ(json["Id"], "cable0");
    EXPECT_EQ(json["Name"], "Cable");
    EXPECT_EQ(json["Status"]["State"], resource::State::Enabled);
    EXPECT_EQ(json["Status"]["Health"], resource::Health::OK);
}

} // namespace
} // namespace redfish
