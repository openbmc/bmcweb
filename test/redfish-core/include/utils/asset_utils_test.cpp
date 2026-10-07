// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "utils/asset_utils.hpp"

#include <boost/asio/error.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/linux_error.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish::asset_utils
{
namespace
{

dbus::utility::DBusPropertiesMap makeAssetProperties(
    const std::string& sparePartNumber)
{
    return {{"Manufacturer", std::string("Acme")},
            {"Model", std::string("Model1")},
            {"PartNumber", std::string("PN1")},
            {"SerialNumber", std::string("SN1")},
            {"SparePartNumber", sparePartNumber}};
}

TEST(AssetUtils, AllPropertiesAreCopied)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    extractAssetInfo(asyncResp, ""_json_pointer, makeAssetProperties("SPN1"),
                     true);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Manufacturer"], "Acme");
    EXPECT_EQ(asyncResp->res.jsonValue["Model"], "Model1");
    EXPECT_EQ(asyncResp->res.jsonValue["PartNumber"], "PN1");
    EXPECT_EQ(asyncResp->res.jsonValue["SerialNumber"], "SN1");
    EXPECT_EQ(asyncResp->res.jsonValue["SparePartNumber"], "SPN1");
}

TEST(AssetUtils, SparePartNumberIsOptedIn)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    extractAssetInfo(asyncResp, ""_json_pointer, makeAssetProperties("SPN1"));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Model"], "Model1");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SparePartNumber"));
}

TEST(AssetUtils, EmptySparePartNumberIsOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    extractAssetInfo(asyncResp, ""_json_pointer, makeAssetProperties(""), true);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SparePartNumber"));
}

TEST(AssetUtils, ManufacturerIsOptedOut)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    extractAssetInfo(asyncResp, ""_json_pointer, makeAssetProperties("SPN1"),
                     false, false);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Manufacturer"));
    EXPECT_EQ(asyncResp->res.jsonValue["Model"], "Model1");
}

TEST(AssetUtils, AbsentPropertiesAreOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {
        {"Model", std::string("Model1")}};

    extractAssetInfo(asyncResp, ""_json_pointer, properties, true);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Model"], "Model1");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Manufacturer"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("PartNumber"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SerialNumber"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SparePartNumber"));
}

TEST(AssetUtils, WrongPropertyTypeSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    dbus::utility::DBusPropertiesMap properties = {{"Model", uint32_t(1)}};

    extractAssetInfo(asyncResp, ""_json_pointer, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AssetUtils, PropertiesUseJsonPointer)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    extractAssetInfo(asyncResp, "/Nested"_json_pointer,
                     makeAssetProperties("SPN1"));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Nested"]["Model"], "Model1");
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Model"));
}

TEST(AssetUtils, EbadrOmitsAssetInfo)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::linux_error::bad_request_descriptor;

    afterGetAssetInfo(asyncResp, ""_json_pointer, false, true, ec,
                      makeAssetProperties("SPN1"));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Model"));
}

TEST(AssetUtils, ErrorSetsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec = boost::asio::error::invalid_argument;

    afterGetAssetInfo(asyncResp, ""_json_pointer, false, true, ec,
                      makeAssetProperties("SPN1"));

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AssetUtils, SuccessPopulatesAssetInfo)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetAssetInfo(asyncResp, ""_json_pointer, false, true, ec,
                      makeAssetProperties("SPN1"));

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["Model"], "Model1");
}

} // namespace
} // namespace redfish::asset_utils
