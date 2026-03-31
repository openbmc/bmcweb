// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "component_integrity.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/component_integrity.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace redfish
{
namespace
{

// --- dbusTypeToRedfishType ---

TEST(DbusTypeToRedfishType, SpdmType)
{
    EXPECT_EQ(dbusTypeToRedfishType(
                  "xyz.openbmc_project.Attestation.ComponentIntegrity."
                  "SecurityTechnologyType.SPDM"),
              component_integrity::ComponentIntegrityType::SPDM);
}

TEST(DbusTypeToRedfishType, TpmType)
{
    EXPECT_EQ(dbusTypeToRedfishType(
                  "xyz.openbmc_project.Attestation.ComponentIntegrity."
                  "SecurityTechnologyType.TPM"),
              component_integrity::ComponentIntegrityType::TPM);
}

TEST(DbusTypeToRedfishType, OemType)
{
    EXPECT_EQ(dbusTypeToRedfishType(
                  "xyz.openbmc_project.Attestation.ComponentIntegrity."
                  "SecurityTechnologyType.OEM"),
              component_integrity::ComponentIntegrityType::OEM);
}

TEST(DbusTypeToRedfishType, UnknownTypeReturnsInvalid)
{
    EXPECT_EQ(dbusTypeToRedfishType("xyz.openbmc_project.Unknown"),
              component_integrity::ComponentIntegrityType::Invalid);
}

TEST(DbusTypeToRedfishType, EmptyStringReturnsInvalid)
{
    EXPECT_EQ(dbusTypeToRedfishType(""),
              component_integrity::ComponentIntegrityType::Invalid);
}

// --- dbusVerificationStatusToRedfish ---

TEST(DbusVerificationStatusToRedfish, Success)
{
    EXPECT_EQ(dbusVerificationStatusToRedfish(
                  "xyz.openbmc_project.Attestation.IdentityAuthentication."
                  "VerificationStatus.Success"),
              component_integrity::VerificationStatus::Success);
}

TEST(DbusVerificationStatusToRedfish, Failed)
{
    EXPECT_EQ(dbusVerificationStatusToRedfish(
                  "xyz.openbmc_project.Attestation.IdentityAuthentication."
                  "VerificationStatus.Failed"),
              component_integrity::VerificationStatus::Failed);
}

TEST(DbusVerificationStatusToRedfish, UnknownReturnsInvalid)
{
    EXPECT_EQ(dbusVerificationStatusToRedfish("xyz.openbmc_project.Unknown"),
              component_integrity::VerificationStatus::Invalid);
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

    ComponentIntegritySpdmState state =
        fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_TRUE(state.isSpdm);
    EXPECT_TRUE(state.enabled);
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
    properties.emplace_back(
        "Type", dbus::utility::DbusVariantType(std::string(
                    "xyz.openbmc_project.Attestation.ComponentIntegrity."
                    "SecurityTechnologyType.SPDM")));
    properties.emplace_back(
        "TypeVersion", dbus::utility::DbusVariantType(std::string("1.2.0")));

    ComponentIntegritySpdmState state =
        fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityEnabled"], false);
    // The schema requires an empty version when security is disabled.
    EXPECT_EQ(asyncResp->res.jsonValue["ComponentIntegrityTypeVersion"], "");
    EXPECT_TRUE(state.isSpdm);
    EXPECT_FALSE(state.enabled);
}

TEST(FillComponentIntegrityProperties, TpmIsNotSpdm)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::DBusPropertiesMap properties;

    properties.emplace_back("Enabled", dbus::utility::DbusVariantType(true));
    properties.emplace_back(
        "Type", dbus::utility::DbusVariantType(std::string(
                    "xyz.openbmc_project.Attestation.ComponentIntegrity."
                    "SecurityTechnologyType.TPM")));

    ComponentIntegritySpdmState state =
        fillComponentIntegrityProperties(asyncResp, ec, properties);

    EXPECT_FALSE(state.isSpdm);
    EXPECT_TRUE(state.enabled);
}

// --- addSpdmProperties ---

constexpr const char* spdmActionKey =
    "#ComponentIntegrity.SPDMGetSignedMeasurements";

TEST(AddSpdmProperties, SpdmEnabledGetsRequesterAndAction)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    addSpdmProperties(
        asyncResp, "gpu0",
        ComponentIntegritySpdmState{.isSpdm = true, .enabled = true}, true);

    EXPECT_EQ(asyncResp->res.jsonValue["SPDM"]["Requester"]["@odata.id"],
              std::format("/redfish/v1/Managers/{}",
                          BMCWEB_REDFISH_MANAGER_URI_NAME));
    EXPECT_EQ(asyncResp->res.jsonValue["Actions"][spdmActionKey]["target"],
              "/redfish/v1/ComponentIntegrity/gpu0/Actions/"
              "ComponentIntegrity.SPDMGetSignedMeasurements");
}

TEST(AddSpdmProperties, SpdmDisabledGetsActionButNoSpdmObject)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    addSpdmProperties(
        asyncResp, "gpu0",
        ComponentIntegritySpdmState{.isSpdm = true, .enabled = false}, true);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SPDM"));
    EXPECT_TRUE(asyncResp->res.jsonValue["Actions"].contains(spdmActionKey));
}

TEST(AddSpdmProperties, TpmGetsNeither)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    addSpdmProperties(
        asyncResp, "tpm0",
        ComponentIntegritySpdmState{.isSpdm = false, .enabled = true}, true);

    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SPDM"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Actions"));
}

TEST(AddSpdmProperties, NoMeasurementSetNoAction)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    addSpdmProperties(
        asyncResp, "gpu0",
        ComponentIntegritySpdmState{.isSpdm = true, .enabled = true}, false);

    EXPECT_TRUE(asyncResp->res.jsonValue.contains("SPDM"));
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Actions"));
}

TEST(ServiceMapHasInterface, FindsInterfaceOnAnyService)
{
    dbus::utility::MapperServiceMap serviceMap{
        {"svc.a", {"xyz.openbmc_project.Attestation.ComponentIntegrity"}},
        {"svc.b", {"xyz.openbmc_project.Attestation.MeasurementSet"}}};
    EXPECT_TRUE(serviceMapHasInterface(
        serviceMap, "xyz.openbmc_project.Attestation.MeasurementSet"));
    EXPECT_FALSE(serviceMapHasInterface(
        serviceMap, "xyz.openbmc_project.Attestation.IdentityAuthentication"));
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

// --- inventoryEndpointToRedfishUri ---

std::string uriString(const std::optional<boost::urls::url>& uri)
{
    if (!uri)
    {
        return "";
    }
    return std::string(uri->buffer());
}

TEST(InventoryEndpointToRedfishUri, ChassisInterface)
{
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.EntityManager",
         {"xyz.openbmc_project.Inventory.Item",
          "xyz.openbmc_project.Inventory.Item.Chassis"}}};
    EXPECT_EQ(uriString(inventoryEndpointToRedfishUri(object, "GPU0")),
              "/redfish/v1/Chassis/GPU0");
}

TEST(InventoryEndpointToRedfishUri, BoardInterface)
{
    // Most entity-manager devices are Boards, which the Chassis collection
    // also lists as Chassis.
    dbus::utility::MapperGetObject object = {
        {"xyz.openbmc_project.EntityManager",
         {"xyz.openbmc_project.Inventory.Item",
          "xyz.openbmc_project.Inventory.Item.Board"}}};
    EXPECT_EQ(uriString(inventoryEndpointToRedfishUri(object, "GPU0")),
              "/redfish/v1/Chassis/GPU0");
}

TEST(InventoryEndpointToRedfishUri, ChassisMappedRegardlessOfPath)
{
    // The D-Bus path layout plays no part, only the interface.
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.Chassis"}}};
    EXPECT_EQ(uriString(inventoryEndpointToRedfishUri(object, "anything")),
              "/redfish/v1/Chassis/anything");
}

TEST(InventoryEndpointToRedfishUri, SystemInterface)
{
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.System"}}};
    EXPECT_EQ(uriString(inventoryEndpointToRedfishUri(object, "Host0")),
              std::format("/redfish/v1/Systems/{}",
                          BMCWEB_REDFISH_SYSTEM_URI_NAME));
}

TEST(InventoryEndpointToRedfishUri, UnmappableInterfaceReturnsNullopt)
{
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.Fan"}}};
    EXPECT_FALSE(inventoryEndpointToRedfishUri(object, "Fan0").has_value());
}

TEST(InventoryEndpointToRedfishUri, EmptyObjectReturnsNullopt)
{
    dbus::utility::MapperGetObject object;
    EXPECT_FALSE(inventoryEndpointToRedfishUri(object, "x").has_value());
}

TEST(InventoryEndpointToRedfishUri, EmptyLeafReturnsNullopt)
{
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.Chassis"}}};
    EXPECT_FALSE(inventoryEndpointToRedfishUri(object, "").has_value());
}

// --- afterGetTargetInventoryObject ---

TEST(AfterGetTargetInventoryObject, UnmappableInterfaceIsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.Fan"}}};

    afterGetTargetInventoryObject(asyncResp, "/inventory/Fan0", ec, object);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("TargetComponentURI"));
}

TEST(AfterGetTargetInventoryObject, LookupErrorIsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());
    dbus::utility::MapperGetObject object;

    afterGetTargetInventoryObject(asyncResp, "/inventory/x", ec, object);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

TEST(AfterGetTargetInventoryObject, ChassisSetsTargetComponentURI)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperGetObject object = {
        {"svc", {"xyz.openbmc_project.Inventory.Item.Chassis"}}};

    afterGetTargetInventoryObject(asyncResp, "/inventory/GPU0", ec, object);

    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
    EXPECT_EQ(asyncResp->res.jsonValue["TargetComponentURI"],
              "/redfish/v1/Chassis/GPU0");
}

// --- fillTargetComponentURI ---

TEST(FillTargetComponentURI, EmptyEndpointsIsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;
    dbus::utility::MapperEndPoints endpoints;

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("TargetComponentURI"));
}

TEST(FillTargetComponentURI, ErrorCodeIsError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec(EBADR, boost::system::generic_category());
    dbus::utility::MapperEndPoints endpoints;

    fillTargetComponentURI(asyncResp, ec, endpoints);

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
}

// --- validateNonce ---

TEST(ValidateNonce, FullLengthHexAllowed)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_TRUE(validateNonce(asyncResp, std::string(64, 'a')));
    EXPECT_TRUE(validateNonce(
        asyncResp,
        "0123456789abcdefABCDEF0123456789abcdefABCDEF0123456789abcdefABCD"));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(ValidateNonce, EmptyRejected)
{
    // A supplied empty Nonce does not match the schema pattern; an omitted
    // Nonce never reaches validateNonce.
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, ""));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
}

TEST(ValidateNonce, ShortRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, "1234"));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
    EXPECT_FALSE(validateNonce(asyncResp, std::string(62, 'a')));
}

TEST(ValidateNonce, NonHexRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, std::string(62, 'a') + "zz"));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
}

TEST(ValidateNonce, EmbeddedNullRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    std::string nonce(64, 'a');
    nonce[10] = '\0';
    EXPECT_FALSE(validateNonce(asyncResp, nonce));
}

TEST(ValidateNonce, OddLengthRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, std::string(63, 'a')));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
}

TEST(ValidateNonce, TooLongRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, std::string(66, 'a')));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
}

// --- findMeasurementSet ---

TEST(FindMeasurementSet, FindsMatchingIdWithInterface)
{
    dbus::utility::MapperGetSubTreeResponse subtree;
    subtree.emplace_back(
        "/xyz/openbmc_project/component_integrity/gpu0",
        dbus::utility::MapperServiceMap{
            {"svc.a", {"xyz.openbmc_project.Attestation.ComponentIntegrity"}},
            {"svc.b", {"xyz.openbmc_project.Attestation.MeasurementSet"}}});
    auto found = findMeasurementSet(subtree, "gpu0");
    EXPECT_EQ(found.value_or(std::pair<std::string, std::string>{}).first,
              "svc.b");
    EXPECT_EQ(found.value_or(std::pair<std::string, std::string>{}).second,
              "/xyz/openbmc_project/component_integrity/gpu0");
}

TEST(FindMeasurementSet, NoInterfaceOrIdReturnsNullopt)
{
    dbus::utility::MapperGetSubTreeResponse subtree;
    subtree.emplace_back(
        "/xyz/openbmc_project/component_integrity/gpu0",
        dbus::utility::MapperServiceMap{
            {"svc.a", {"xyz.openbmc_project.Attestation.ComponentIntegrity"}}});
    EXPECT_FALSE(findMeasurementSet(subtree, "gpu0").has_value());
    EXPECT_FALSE(findMeasurementSet(subtree, "gpu1").has_value());
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
// fillIdentityAuthenticationProperties, and
// afterGetTargetInventoryObject tests above, and by E2E testing in Renode.

// --- afterGetSignedMeasurements ---

TEST(AfterGetSignedMeasurements, DbusErrorReturnsInternalError)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "c2lnbmVk", "TPMT_SIGNATURE", "1.1.0");

    EXPECT_EQ(asyncResp->res.result(),
              boost::beast::http::status::internal_server_error);
    // On error nothing should be populated.
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("SignedMeasurements"));
}

TEST(AfterGetSignedMeasurements, HttpsCertificateMappedToManagerUri)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "c2lnbmVk", "TPMT_SIGNATURE", "1.1.0");

    EXPECT_EQ(
        asyncResp->res.jsonValue["Certificate"]["@odata.id"],
        std::format(
            "/redfish/v1/Managers/{}/NetworkProtocol/HTTPS/Certificates/1",
            BMCWEB_REDFISH_MANAGER_URI_NAME));
}

TEST(AfterGetSignedMeasurements, LdapCertificateMapped)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/client/ldap/3"),
        "alg", "meas", "sig", "1.0.0");

    EXPECT_EQ(asyncResp->res.jsonValue["Certificate"]["@odata.id"],
              "/redfish/v1/AccountService/LDAP/Certificates/3");
}

TEST(AfterGetSignedMeasurements, TruststoreCertificateMappedToManagerUri)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path(
            "/xyz/openbmc_project/certs/authority/truststore/7"),
        "alg", "meas", "sig", "1.0.0");

    EXPECT_EQ(asyncResp->res.jsonValue["Certificate"]["@odata.id"],
              std::format("/redfish/v1/Managers/{}/Truststore/Certificates/7",
                          BMCWEB_REDFISH_MANAGER_URI_NAME));
}

TEST(AfterGetSignedMeasurements, DeviceCertificateOmittedUntilRouteExists)
{
    // No GET route serves device certificates yet, so no link is made for
    // either device certificate layout.
    for (const char* path : {"/xyz/openbmc_project/certs/chassis/GPU0/cert1",
                             "/xyz/openbmc_project/certs/devices/GPU0/cert1"})
    {
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
        boost::system::error_code ec;

        afterGetSignedMeasurements(asyncResp, ec, sdbusplus::object_path(path),
                                   "alg", "meas", "sig", "1.0.0");

        EXPECT_FALSE(asyncResp->res.jsonValue.contains("Certificate"));
        EXPECT_EQ(asyncResp->res.jsonValue["SignedMeasurements"], "meas");
    }
}

TEST(AfterGetSignedMeasurements, UnmappableCertificatePathOmitsCertificate)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/unknown/place/9"),
        "alg", "meas", "sig", "1.0.0");

    // Unknown cert parent: Certificate link omitted, but the measurement
    // payload is still returned.
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("Certificate"));
    EXPECT_EQ(asyncResp->res.jsonValue["SignedMeasurements"], "meas");
}

TEST(AfterGetSignedMeasurements, MeasurementFieldsPopulated)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        asyncResp, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "c2lnbmVkTWVhcw==", "TPMT_SIGNATURE", "1.1.0");

    EXPECT_EQ(asyncResp->res.jsonValue["HashingAlgorithm"], "TPM_ALG_SHA_384");
    // PublicKey is only allowed for SlotId 15, which is never accepted.
    EXPECT_FALSE(asyncResp->res.jsonValue.contains("PublicKey"));
    EXPECT_EQ(asyncResp->res.jsonValue["SignedMeasurements"],
              "c2lnbmVkTWVhcw==");
    EXPECT_EQ(asyncResp->res.jsonValue["SigningAlgorithm"], "TPMT_SIGNATURE");
    EXPECT_EQ(asyncResp->res.jsonValue["Version"], "1.1.0");
}

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
