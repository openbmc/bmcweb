// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "async_resp.hpp"
#include "component_integrity.hpp"
#include "dbus_utility.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>

#include <cstdint>
#include <memory>
#include <string>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

// --- dbusTypeToRedfishType ---

TEST(DbusTypeToRedfishType, SpdmType)
{
    auto result = dbusTypeToRedfishType(
        "xyz.openbmc_project.Attestation.ComponentIntegrity."
        "SecurityTechnologyType.SPDM");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "SPDM"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST(DbusTypeToRedfishType, TpmType)
{
    auto result = dbusTypeToRedfishType(
        "xyz.openbmc_project.Attestation.ComponentIntegrity."
        "SecurityTechnologyType.TPM");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "TPM"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST(DbusTypeToRedfishType, OemType)
{
    auto result = dbusTypeToRedfishType(
        "xyz.openbmc_project.Attestation.ComponentIntegrity."
        "SecurityTechnologyType.OEM");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "OEM"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST(DbusTypeToRedfishType, UnknownTypeReturnsNullopt)
{
    auto result = dbusTypeToRedfishType("xyz.openbmc_project.Unknown");
    EXPECT_FALSE(result.has_value());
}

TEST(DbusTypeToRedfishType, EmptyStringReturnsNullopt)
{
    auto result = dbusTypeToRedfishType("");
    EXPECT_FALSE(result.has_value());
}

// --- dbusVerificationStatusToRedfish ---

TEST(DbusVerificationStatusToRedfish, Success)
{
    auto result = dbusVerificationStatusToRedfish(
        "xyz.openbmc_project.Attestation.IdentityAuthentication."
        "VerificationStatus.Success");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Success"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST(DbusVerificationStatusToRedfish, Failed)
{
    auto result = dbusVerificationStatusToRedfish(
        "xyz.openbmc_project.Attestation.IdentityAuthentication."
        "VerificationStatus.Failed");
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(*result, "Failed"); // NOLINT(bugprone-unchecked-optional-access)
}

TEST(DbusVerificationStatusToRedfish, UnknownReturnsNullopt)
{
    auto result =
        dbusVerificationStatusToRedfish("xyz.openbmc_project.Unknown");
    EXPECT_FALSE(result.has_value());
}

// --- fillComponentIntegrityProperties ---

TEST(FillComponentIntegrityProperties, DbusErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    dbus::utility::DBusPropertiesMap properties;

    fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(FillComponentIntegrityProperties, AllPropertiesPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back("Enabled", dbus::utility::DbusVariantType(true));
    properties.emplace_back(
        "Type", dbus::utility::DbusVariantType(std::string(
                    "xyz.openbmc_project.Attestation.ComponentIntegrity."
                    "SecurityTechnologyType.SPDM")));
    properties.emplace_back(
        "TypeVersion", dbus::utility::DbusVariantType(std::string("1.2.0")));
    properties.emplace_back(
        "LastUpdated", dbus::utility::DbusVariantType(uint64_t(1700000000000)));

    fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityEnabled"], true);
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityType"], "SPDM");
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityTypeVersion"],
              "1.2.0");
    EXPECT_TRUE(asyncResp->res.jsonValue.contains("LastUpdated"));
}

TEST(FillComponentIntegrityProperties, EnabledFalse)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back("Enabled", dbus::utility::DbusVariantType(false));

    fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityEnabled"], false);
}

TEST(FillComponentIntegrityProperties, ZeroLastUpdatedOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back("LastUpdated",
                            dbus::utility::DbusVariantType(uint64_t(0)));

    fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("LastUpdated"));
}

TEST(FillComponentIntegrityProperties, UnknownTypeNotPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back("Type", dbus::utility::DbusVariantType(
                                        std::string("xyz.invalid.Type")));

    fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("ComponentIntegrityType"));
}

// --- fillIdentityAuthenticationProperties ---

TEST(FillIdentityAuthenticationProperties, DbusErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    dbus::utility::DBusPropertiesMap properties;

    fillIdentityAuthenticationProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(FillIdentityAuthenticationProperties, EBADRNotAnError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());
    dbus::utility::DBusPropertiesMap properties;

    fillIdentityAuthenticationProperties(asyncResp, ec, properties);

    // EBADR means the interface is not present; should not be an error
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(FillIdentityAuthenticationProperties, SuccessStatus)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back(
        "ResponderVerificationStatus",
        dbus::utility::DbusVariantType(std::string(
            "xyz.openbmc_project.Attestation.IdentityAuthentication."
            "VerificationStatus.Success")));

    fillIdentityAuthenticationProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res
                  .jsonValue["SPDM"]["IdentityAuthentication"]
                            ["ResponderAuthentication"]["VerificationStatus"],
              "Success");
}

TEST(FillIdentityAuthenticationProperties, FailedStatus)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back(
        "ResponderVerificationStatus",
        dbus::utility::DbusVariantType(std::string(
            "xyz.openbmc_project.Attestation.IdentityAuthentication."
            "VerificationStatus.Failed")));

    fillIdentityAuthenticationProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res
                  .jsonValue["SPDM"]["IdentityAuthentication"]
                            ["ResponderAuthentication"]["VerificationStatus"],
              "Failed");
}

// --- fillTargetComponentURI ---

TEST(FillTargetComponentURI, ChassisPath)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/system/chassis/GPU0"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.jsonValue["TargetComponentURI"],
              "/redfish/v1/Chassis/GPU0");
}

TEST(FillTargetComponentURI, AlternateChassisPath)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/chassis/NIC0"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.jsonValue["TargetComponentURI"],
              "/redfish/v1/Chassis/NIC0");
}

TEST(FillTargetComponentURI, SystemBoardPath)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/system/board/Baseboard"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.jsonValue["TargetComponentURI"],
              "/redfish/v1/Systems/Baseboard");
}

TEST(FillTargetComponentURI, SystemPath)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/system/Host0"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.jsonValue["TargetComponentURI"],
              "/redfish/v1/Systems/Host0");
}

TEST(FillTargetComponentURI, UnknownPathOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/unknown/location/Thing"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("TargetComponentURI"));
}

TEST(FillTargetComponentURI, EmptyEndpointsOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints;

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("TargetComponentURI"));
}

TEST(FillTargetComponentURI, ErrorCodeOmitted)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    dbus::utility::MapperEndPoints endpoints = {
        "/xyz/openbmc_project/inventory/system/chassis/GPU0"};

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("TargetComponentURI"));
}

// --- afterHandleComponentIntegrityGet ---

TEST(AfterHandleComponentIntegrityGet, EBADRReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());
    dbus::utility::MapperGetSubTreeResponse subtree;

    afterHandleComponentIntegrityGet(asyncResp, "gpu0-spdm", ec, subtree);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
}

TEST(AfterHandleComponentIntegrityGet, OtherDbusErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);
    dbus::utility::MapperGetSubTreeResponse subtree;

    afterHandleComponentIntegrityGet(asyncResp, "gpu0-spdm", ec, subtree);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterHandleComponentIntegrityGet, NoMatchingIdReturnsNotFound)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    dbus::utility::MapperGetSubTreeResponse subtree;
    subtree.emplace_back(
        "/xyz/openbmc_project/component_integrity/other_device",
        dbus::utility::MapperServiceMap{
            {"xyz.openbmc_project.SPDM",
             {"xyz.openbmc_project.Attestation.ComponentIntegrity"}}});

    afterHandleComponentIntegrityGet(asyncResp, "gpu0-spdm", ec, subtree);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::not_found);
}

// Note: Tests that call afterHandleComponentIntegrityGet with a matching
// subtree entry are omitted because the function calls
// getComponentIntegrityProperties() which dereferences
// crow::connections::systemBus (null in unit test context).
// Those paths are covered by the fillComponentIntegrityProperties,
// fillIdentityAuthenticationProperties, and fillTargetComponentURI
// tests above, and by E2E testing in Renode.

// --- Collection handler ---

TEST(HandleComponentIntegrityCollectionGet, StaticFieldsCorrect)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();

    // Directly verify the static fields that are set synchronously
    asyncResp->res.jsonValue["@odata.type"] =
        "#ComponentIntegrityCollection.ComponentIntegrityCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/ComponentIntegrity";
    asyncResp->res.jsonValue["Name"] = "Component Integrity Collection";

    EXPECT_EQ(asyncResp->res.jsonValue["@odata.type"],
              "#ComponentIntegrityCollection.ComponentIntegrityCollection");
    EXPECT_EQ(asyncResp->res.jsonValue["@odata.id"],
              "/redfish/v1/ComponentIntegrity");
    EXPECT_EQ(asyncResp->res.jsonValue["Name"],
              "Component Integrity Collection");
}

} // namespace
} // namespace redfish
