// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "bmcweb_config.h"

#include "async_resp.hpp"
#include "component_integrity.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/component_integrity.hpp"
#include "http_response.hpp"
#include "io_context_singleton.hpp"
#include "task.hpp"

#include <asm-generic/errno.h>

#include <boost/asio/io_context.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <chrono>
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

TEST(ValidateNonce, EmptyAllowed)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_TRUE(validateNonce(asyncResp, ""));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::ok);
}

TEST(ValidateNonce, FullLengthHexAllowed)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_TRUE(validateNonce(asyncResp, std::string(64, 'a')));
    EXPECT_TRUE(validateNonce(asyncResp, "0123456789abcdefABCDEF00"));
}

TEST(ValidateNonce, NonHexRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, "zz"));
    EXPECT_EQ(asyncResp->res.result(), boost::beast::http::status::bad_request);
}

TEST(ValidateNonce, EmbeddedNullRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, std::string("ab\0c", 4)));
}

TEST(ValidateNonce, OddLengthRejected)
{
    auto asyncResp = std::make_shared<bmcweb::AsyncResp>();
    EXPECT_FALSE(validateNonce(asyncResp, "abc"));
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
    crow::Response res;
    boost::system::error_code ec =
        boost::system::errc::make_error_code(boost::system::errc::io_error);

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "publicKeyPem", "c2lnbmVk", "TPMT_SIGNATURE",
        "1.1.0");

    EXPECT_EQ(res.result(), boost::beast::http::status::internal_server_error);
    // On error nothing should be populated.
    EXPECT_FALSE(res.jsonValue.contains("SignedMeasurements"));
}

TEST(AfterGetSignedMeasurements, HttpsCertificateMappedToManagerUri)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "publicKeyPem", "c2lnbmVk", "TPMT_SIGNATURE",
        "1.1.0");

    EXPECT_EQ(
        res.jsonValue["Certificate"]["@odata.id"],
        std::format(
            "/redfish/v1/Managers/{}/NetworkProtocol/HTTPS/Certificates/1",
            BMCWEB_REDFISH_MANAGER_URI_NAME));
}

TEST(AfterGetSignedMeasurements, LdapCertificateMapped)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/client/ldap/3"),
        "alg", "key", "meas", "sig", "1.0.0");

    EXPECT_EQ(res.jsonValue["Certificate"]["@odata.id"],
              "/redfish/v1/AccountService/LDAP/Certificates/3");
}

TEST(AfterGetSignedMeasurements, TruststoreCertificateMappedToManagerUri)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path(
            "/xyz/openbmc_project/certs/authority/truststore/7"),
        "alg", "key", "meas", "sig", "1.0.0");

    EXPECT_EQ(res.jsonValue["Certificate"]["@odata.id"],
              std::format("/redfish/v1/Managers/{}/Truststore/Certificates/7",
                          BMCWEB_REDFISH_MANAGER_URI_NAME));
}

TEST(AfterGetSignedMeasurements, DeviceCertificateMappedToChassis)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/chassis/GPU0/cert1"),
        "alg", "key", "meas", "sig", "1.0.0");

    EXPECT_EQ(res.jsonValue["Certificate"]["@odata.id"],
              "/redfish/v1/Chassis/GPU0/Certificates/cert1");
}

TEST(AfterGetSignedMeasurements, UnmappableCertificatePathOmitsCertificate)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/unknown/place/9"),
        "alg", "key", "meas", "sig", "1.0.0");

    // Unknown cert parent: Certificate link omitted, but the measurement
    // payload is still returned.
    EXPECT_FALSE(res.jsonValue.contains("Certificate"));
    EXPECT_EQ(res.jsonValue["SignedMeasurements"], "meas");
}

TEST(AfterGetSignedMeasurements, MeasurementFieldsPopulated)
{
    crow::Response res;
    boost::system::error_code ec;

    afterGetSignedMeasurements(
        res, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "publicKeyPem", "c2lnbmVkTWVhcw==", "TPMT_SIGNATURE",
        "1.1.0");

    EXPECT_EQ(res.jsonValue["HashingAlgorithm"], "TPM_ALG_SHA_384");
    EXPECT_EQ(res.jsonValue["PublicKey"], "publicKeyPem");
    EXPECT_EQ(res.jsonValue["SignedMeasurements"], "c2lnbmVkTWVhcw==");
    EXPECT_EQ(res.jsonValue["SigningAlgorithm"], "TPMT_SIGNATURE");
    EXPECT_EQ(res.jsonValue["Version"], "1.1.0");
}

// --- completeSignedMeasurementsTask ---

void runPendingHandlers()
{
    getIoContext().restart();
    getIoContext().run_for(std::chrono::seconds(5));
}

std::shared_ptr<task::TaskData> startSignedMeasurementsTask(
    std::chrono::seconds timeout)
{
    std::shared_ptr<task::TaskData> taskData =
        task::TaskData::createTask(handleSignedMeasurementsTaskTimeout, "");
    taskData->startTimer(timeout);
    return taskData;
}

void replySignedMeasurements(const std::shared_ptr<task::TaskData>& taskData,
                             const boost::system::error_code& ec)
{
    completeSignedMeasurementsTask(
        taskData, ec,
        sdbusplus::object_path("/xyz/openbmc_project/certs/server/https/1"),
        "TPM_ALG_SHA_384", "publicKeyPem", "c2lnbmVk", "TPMT_SIGNATURE",
        "1.2.0");
}

class SignedMeasurementsTaskTest : public ::testing::Test
{
  protected:
    // Remove the tasks created by this test
    void TearDown() override
    {
        for (const std::shared_ptr<task::TaskData>& taskData : task::tasks)
        {
            taskData->finishTask();
            taskData->timer.cancel();
        }
        runPendingHandlers();
        task::tasks.clear();
    }
};

TEST_F(SignedMeasurementsTaskTest, ReplyCompletesTaskWithResponse)
{
    std::shared_ptr<task::TaskData> taskData =
        startSignedMeasurementsTask(signedMeasurementsTimeout);
    replySignedMeasurements(taskData, {});
    runPendingHandlers();

    EXPECT_EQ(taskData->state, "Completed");
    EXPECT_EQ(taskData->status, "OK");
    EXPECT_EQ(taskData->percentComplete, 100);

    crow::Response monitor;
    taskData->populateResp(monitor);
    EXPECT_EQ(monitor.result(), boost::beast::http::status::ok);
    EXPECT_EQ(monitor.jsonValue["SignedMeasurements"], "c2lnbmVk");
    EXPECT_EQ(monitor.jsonValue["Version"], "1.2.0");
}

TEST_F(SignedMeasurementsTaskTest, DbusErrorEndsTaskWithError)
{
    std::shared_ptr<task::TaskData> taskData =
        startSignedMeasurementsTask(signedMeasurementsTimeout);
    replySignedMeasurements(taskData, boost::system::errc::make_error_code(
                                          boost::system::errc::io_error));
    runPendingHandlers();

    EXPECT_EQ(taskData->state, "Exception");
    EXPECT_EQ(taskData->status, "Warning");

    crow::Response monitor;
    taskData->populateResp(monitor);
    EXPECT_EQ(monitor.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_TRUE(monitor.jsonValue.contains("error"));
    EXPECT_FALSE(monitor.jsonValue.contains("SignedMeasurements"));
}

TEST_F(SignedMeasurementsTaskTest, TimeoutEndsTaskAndIgnoresLateReply)
{
    std::shared_ptr<task::TaskData> taskData =
        startSignedMeasurementsTask(std::chrono::seconds(0));
    runPendingHandlers();

    EXPECT_EQ(taskData->state, "Cancelled");
    replySignedMeasurements(taskData, {});
    EXPECT_EQ(taskData->state, "Cancelled");

    crow::Response monitor;
    taskData->populateResp(monitor);
    EXPECT_EQ(monitor.result(),
              boost::beast::http::status::internal_server_error);
    EXPECT_FALSE(monitor.jsonValue.contains("SignedMeasurements"));
}

TEST_F(SignedMeasurementsTaskTest, TasksKeepTheirOwnResults)
{
    std::shared_ptr<task::TaskData> first =
        startSignedMeasurementsTask(signedMeasurementsTimeout);
    std::shared_ptr<task::TaskData> second =
        startSignedMeasurementsTask(signedMeasurementsTimeout);
    replySignedMeasurements(second, boost::system::errc::make_error_code(
                                        boost::system::errc::io_error));
    replySignedMeasurements(first, {});
    runPendingHandlers();

    crow::Response firstMonitor;
    first->populateResp(firstMonitor);
    EXPECT_EQ(firstMonitor.result(), boost::beast::http::status::ok);

    crow::Response secondMonitor;
    second->populateResp(secondMonitor);
    EXPECT_EQ(secondMonitor.result(),
              boost::beast::http::status::internal_server_error);
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
