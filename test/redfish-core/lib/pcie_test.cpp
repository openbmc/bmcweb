// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages_test_utils.hpp"
#include "generated/enums/pcie_function.hpp"
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
    EXPECT_EQ(response->res.jsonValue, internalErrorJson());
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

TEST(AddPCIeFunctionPropertiesFromInterface, ValidEnumsArePublished)
{
    crow::Response resp;
    dbus::utility::DBusPropertiesMap properties = {
        {"FunctionType",
         std::string("xyz.openbmc_project.Inventory.Item.PCIeFunction."
                     "FunctionTypes.Physical")},
        {"DeviceClass",
         std::string("xyz.openbmc_project.Inventory.Item.PCIeFunction."
                     "DeviceClasses.MassStorageController")}};

    addPCIeFunctionPropertiesFromInterface(resp, properties);

    EXPECT_EQ(resp.jsonValue["FunctionType"],
              pcie_function::FunctionType::Physical);
    EXPECT_EQ(resp.jsonValue["DeviceClass"],
              pcie_function::DeviceClass::MassStorageController);
}

TEST(AddPCIeFunctionPropertiesFromInterface, UnknownEnumsAreOmitted)
{
    crow::Response resp;
    dbus::utility::DBusPropertiesMap properties = {
        {"FunctionType",
         std::string("xyz.openbmc_project.Inventory.Item.PCIeFunction."
                     "FunctionTypes.Unknown")},
        {"DeviceClass",
         std::string("xyz.openbmc_project.Inventory.Item.PCIeFunction."
                     "DeviceClasses.Unknown")}};

    addPCIeFunctionPropertiesFromInterface(resp, properties);

    EXPECT_EQ(resp.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(resp.jsonValue.contains("FunctionType"));
    EXPECT_FALSE(resp.jsonValue.contains("DeviceClass"));
}

TEST(AddPCIeFunctionPropertiesFromInterface, UnmappedEnumSetsInternalError)
{
    crow::Response resp;
    dbus::utility::DBusPropertiesMap properties = {
        {"DeviceClass", std::string("Mass Storage Controller")}};

    addPCIeFunctionPropertiesFromInterface(resp, properties);

    EXPECT_EQ(resp.result(), boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(resp.jsonValue.contains("DeviceClass"));
}

} // namespace
} // namespace redfish
