// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/component_integrity.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/time_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace redfish
{

constexpr std::array<std::string_view, 1> componentIntegrityInterfaces = {
    "xyz.openbmc_project.Attestation.ComponentIntegrity"};

inline component_integrity::ComponentIntegrityType dbusTypeToRedfishType(
    const std::string& dbusType)
{
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.SPDM")
    {
        return component_integrity::ComponentIntegrityType::SPDM;
    }
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.TPM")
    {
        return component_integrity::ComponentIntegrityType::TPM;
    }
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.OEM")
    {
        return component_integrity::ComponentIntegrityType::OEM;
    }
    return component_integrity::ComponentIntegrityType::Invalid;
}

inline component_integrity::VerificationStatus dbusVerificationStatusToRedfish(
    const std::string& dbusStatus)
{
    if (dbusStatus ==
        "xyz.openbmc_project.Attestation.IdentityAuthentication.VerificationStatus.Success")
    {
        return component_integrity::VerificationStatus::Success;
    }
    if (dbusStatus ==
        "xyz.openbmc_project.Attestation.IdentityAuthentication.VerificationStatus.Failed")
    {
        return component_integrity::VerificationStatus::Failed;
    }
    return component_integrity::VerificationStatus::Invalid;
}

inline void fillComponentIntegrityProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error for ComponentIntegrity: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const bool* enabled = nullptr;
    const std::string* type = nullptr;
    const std::string* typeVersion = nullptr;
    const uint64_t* lastUpdated = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Enabled", enabled,
        "Type", type, "TypeVersion", typeVersion, "LastUpdated", lastUpdated);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (enabled != nullptr)
    {
        asyncResp->res.jsonValue["ComponentIntegrityEnabled"] = *enabled;
    }

    if (type != nullptr)
    {
        component_integrity::ComponentIntegrityType rfType =
            dbusTypeToRedfishType(*type);
        if (rfType != component_integrity::ComponentIntegrityType::Invalid)
        {
            asyncResp->res.jsonValue["ComponentIntegrityType"] = rfType;
        }
    }

    if (typeVersion != nullptr)
    {
        asyncResp->res.jsonValue["ComponentIntegrityTypeVersion"] =
            *typeVersion;
    }

    if (lastUpdated != nullptr && *lastUpdated != 0)
    {
        asyncResp->res.jsonValue["LastUpdated"] =
            redfish::time_utils::getDateTimeUintMs(*lastUpdated);
    }
}

inline void fillIdentityAuthenticationProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::DBusPropertiesMap& properties)
{
    if (ec)
    {
        if (ec.value() == EBADR)
        {
            // Interface not present, not an error
            return;
        }
        BMCWEB_LOG_ERROR("DBUS response error for IdentityAuthentication: {}",
                         ec);
        messages::internalError(asyncResp->res);
        return;
    }

    const std::string* responderStatus = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties,
        "ResponderVerificationStatus", responderStatus);

    if (!success)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    if (responderStatus != nullptr)
    {
        component_integrity::VerificationStatus rfStatus =
            dbusVerificationStatusToRedfish(*responderStatus);
        if (rfStatus != component_integrity::VerificationStatus::Invalid)
        {
            asyncResp->res
                .jsonValue["SPDM"]["IdentityAuthentication"]
                          ["ResponderAuthentication"]["VerificationStatus"] =
                rfStatus;
        }
    }
}

// Map the inventory interfaces of an "authenticating" association endpoint to
// the Redfish URI of the resource that represents it.  The mapping is by
// inventory interface, never by D-Bus path layout.  Chassis-like interfaces
// (Item.Board and Item.Chassis) are the same set the Chassis collection uses.
inline std::optional<boost::urls::url> inventoryEndpointToRedfishUri(
    const dbus::utility::MapperGetObject& object, const std::string& leaf)
{
    if (leaf.empty())
    {
        return std::nullopt;
    }
    for (const auto& [service, interfaces] : object)
    {
        for (const std::string& interface : interfaces)
        {
            if (std::ranges::find(chassisInterfaces, interface) !=
                chassisInterfaces.end())
            {
                return boost::urls::format("/redfish/v1/Chassis/{}", leaf);
            }
            if (interface == "xyz.openbmc_project.Inventory.Item.System")
            {
                return boost::urls::format("/redfish/v1/Systems/{}",
                                           BMCWEB_REDFISH_SYSTEM_URI_NAME);
            }
        }
    }
    return std::nullopt;
}

inline void afterGetTargetInventoryObject(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& targetPath, const boost::system::error_code& ec,
    const dbus::utility::MapperGetObject& object)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("Cannot look up authenticating endpoint {}: {}",
                         targetPath, ec);
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<boost::urls::url> targetURI = inventoryEndpointToRedfishUri(
        object, sdbusplus::object_path(targetPath).filename());
    if (!targetURI)
    {
        BMCWEB_LOG_ERROR(
            "Authenticating endpoint {} has no inventory interface that maps to a Redfish resource",
            targetPath);
        messages::internalError(asyncResp->res);
        return;
    }
    asyncResp->res.jsonValue["TargetComponentURI"] = *targetURI;
}

// The "authenticating" association is mandatory: TargetComponentURI is
// required by the schema and the SPDM daemon is responsible for providing it.
inline void fillTargetComponentURI(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec || endpoints.empty())
    {
        BMCWEB_LOG_ERROR("Missing authenticating association: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    constexpr std::array<std::string_view, 0> noInterfaces{};
    dbus::utility::getDbusObject(endpoints[0], noInterfaces,
                                 std::bind_front(afterGetTargetInventoryObject,
                                                 asyncResp, endpoints[0]));
}

inline void getComponentIntegrityProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath,
    const dbus::utility::MapperServiceMap& serviceMap)
{
    BMCWEB_LOG_DEBUG("Get properties for ComponentIntegrity {}", objectPath);

    for (const auto& [service, interfaces] : serviceMap)
    {
        for (const auto& interface : interfaces)
        {
            if (interface ==
                "xyz.openbmc_project.Attestation.ComponentIntegrity")
            {
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, service, objectPath,
                    interface,
                    std::bind_front(fillComponentIntegrityProperties,
                                    asyncResp));
            }
            else if (interface ==
                     "xyz.openbmc_project.Attestation.IdentityAuthentication")
            {
                dbus::utility::getAllProperties(
                    *crow::connections::systemBus, service, objectPath,
                    interface,
                    std::bind_front(fillIdentityAuthenticationProperties,
                                    asyncResp));
            }
        }
    }

    // Resolve the "authenticating" association to get TargetComponentURI
    dbus::utility::getAssociationEndPoints(
        objectPath + "/authenticating",
        std::bind_front(fillTargetComponentURI, asyncResp));
}

inline void afterHandleComponentIntegrityGet(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& componentIntegrityId,
    const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "ComponentIntegrity",
                                   componentIntegrityId);
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::object_path path(objectPath);
        if (path.filename() != componentIntegrityId)
        {
            continue;
        }

        asyncResp->res.jsonValue["@odata.type"] =
            "#ComponentIntegrity.v1_3_2.ComponentIntegrity";
        asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
            "/redfish/v1/ComponentIntegrity/{}", componentIntegrityId);
        asyncResp->res.jsonValue["Id"] = componentIntegrityId;
        asyncResp->res.jsonValue["Name"] = "Component Integrity";

        // The BMC is always the SPDM requester
        asyncResp->res.jsonValue["SPDM"]["Requester"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Managers/{}",
                                BMCWEB_REDFISH_MANAGER_URI_NAME);

        // Check if the object supports MeasurementSet for the action
        for (const auto& [service, interfaces] : serviceMap)
        {
            for (const auto& iface : interfaces)
            {
                if (iface == "xyz.openbmc_project.Attestation.MeasurementSet")
                {
                    nlohmann::json& action =
                        asyncResp->res
                            .jsonValue["Actions"]["#ComponentIntegrity."
                                                  "SPDMGetSignedMeasurements"];
                    action["target"] = boost::urls::format(
                        "/redfish/v1/ComponentIntegrity/{}/Actions/"
                        "ComponentIntegrity.SPDMGetSignedMeasurements",
                        componentIntegrityId);
                }
            }
        }

        getComponentIntegrityProperties(asyncResp, objectPath, serviceMap);
        return;
    }
    messages::resourceNotFound(asyncResp->res, "ComponentIntegrity",
                               componentIntegrityId);
}

inline void handleComponentIntegrityGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& componentIntegrityId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    BMCWEB_LOG_DEBUG("ComponentIntegrity Id: {}", componentIntegrityId);

    constexpr std::array<std::string_view, 3> interfaces = {
        "xyz.openbmc_project.Attestation.ComponentIntegrity",
        "xyz.openbmc_project.Attestation.IdentityAuthentication",
        "xyz.openbmc_project.Attestation.MeasurementSet"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/component_integrity", 0, interfaces,
        std::bind_front(afterHandleComponentIntegrityGet, asyncResp,
                        componentIntegrityId));
}

inline void handleComponentIntegrityCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#ComponentIntegrityCollection.ComponentIntegrityCollection";
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/ComponentIntegrity";
    asyncResp->res.jsonValue["Name"] = "Component Integrity Collection";
    asyncResp->res.jsonValue["Description"] =
        "Collection of ComponentIntegrity resources";
    collection_util::getCollectionMembers(
        asyncResp, boost::urls::url("/redfish/v1/ComponentIntegrity"),
        componentIntegrityInterfaces,
        "/xyz/openbmc_project/component_integrity");
}

inline void handleComponentIntegrityCollectionHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComponentIntegrityCollection/ComponentIntegrityCollection.json>; rel=describedby");
}

inline void handleComponentIntegrityHead(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& /*componentIntegrityId*/)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    asyncResp->res.addHeader(
        boost::beast::http::field::link,
        "</redfish/v1/JsonSchemas/ComponentIntegrity/ComponentIntegrity.json>; rel=describedby");
}

inline void afterGetSignedMeasurements(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const sdbusplus::object_path& certificate,
    const std::string& hashingAlgorithm, const std::string& publicKey,
    const std::string& signedMeasurements, const std::string& signingAlgorithm,
    const std::string& version)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("SPDMGetSignedMeasurements D-Bus error: {}", ec);
        messages::internalError(asyncResp->res);
        return;
    }

    // Map certificate D-Bus path to Redfish URI
    sdbusplus::object_path certPath(certificate.str);
    std::string certId = certPath.filename();
    std::string certParent = certPath.parent_path();
    if (certParent == "/xyz/openbmc_project/certs/server/https")
    {
        asyncResp->res.jsonValue["Certificate"]["@odata.id"] =
            boost::urls::format(
                "/redfish/v1/Managers/{}/NetworkProtocol/HTTPS/Certificates/{}",
                BMCWEB_REDFISH_MANAGER_URI_NAME, certId);
    }
    else if (certParent == "/xyz/openbmc_project/certs/client/ldap")
    {
        asyncResp->res.jsonValue["Certificate"]["@odata.id"] =
            boost::urls::format(
                "/redfish/v1/AccountService/LDAP/Certificates/{}", certId);
    }
    else if (sdbusplus::object_path(certParent).parent_path() ==
             "/xyz/openbmc_project/certs/chassis")
    {
        // Device certificates: /certs/chassis/{ChassisId}/{CertId}, per the
        // redfish-spdm-attestation design.
        asyncResp->res.jsonValue["Certificate"]["@odata.id"] =
            boost::urls::format("/redfish/v1/Chassis/{}/Certificates/{}",
                                sdbusplus::object_path(certParent).filename(),
                                certId);
    }
    else if (certParent == "/xyz/openbmc_project/certs/authority/truststore")
    {
        asyncResp->res.jsonValue["Certificate"]["@odata.id"] =
            boost::urls::format(
                "/redfish/v1/Managers/{}/Truststore/Certificates/{}",
                BMCWEB_REDFISH_MANAGER_URI_NAME, certId);
    }
    else
    {
        BMCWEB_LOG_WARNING("Cannot map certificate path {} to Redfish URI",
                           certificate.str);
    }
    asyncResp->res.jsonValue["HashingAlgorithm"] = hashingAlgorithm;
    asyncResp->res.jsonValue["PublicKey"] = publicKey;
    asyncResp->res.jsonValue["SignedMeasurements"] = signedMeasurements;
    asyncResp->res.jsonValue["SigningAlgorithm"] = signingAlgorithm;
    asyncResp->res.jsonValue["Version"] = version;
}

// Find the service that hosts the MeasurementSet interface on the
// ComponentIntegrity object named componentIntegrityId.
inline std::optional<std::pair<std::string, std::string>> findMeasurementSet(
    const dbus::utility::MapperGetSubTreeResponse& subtree,
    const std::string& componentIntegrityId)
{
    for (const auto& [objectPath, serviceMap] : subtree)
    {
        if (sdbusplus::object_path(objectPath).filename() !=
            componentIntegrityId)
        {
            continue;
        }
        for (const auto& [service, interfaces] : serviceMap)
        {
            if (std::ranges::contains(
                    interfaces,
                    "xyz.openbmc_project.Attestation.MeasurementSet"))
            {
                return std::make_pair(service, objectPath);
            }
        }
    }
    return std::nullopt;
}

inline void callSPDMGetSignedMeasurements(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& service, const std::string& objectPath,
    const std::vector<size_t>& measurementIndices, const std::string& nonce,
    size_t slotId)
{
    dbus::utility::async_method_call(
        [asyncResp](
            const boost::system::error_code& ec,
            const sdbusplus::object_path& certificate,
            const std::string& hashingAlgorithm, const std::string& publicKey,
            const std::string& signedMeasurements,
            const std::string& signingAlgorithm, const std::string& version) {
            afterGetSignedMeasurements(
                asyncResp, ec, certificate, hashingAlgorithm, publicKey,
                signedMeasurements, signingAlgorithm, version);
        },
        service, objectPath, "xyz.openbmc_project.Attestation.MeasurementSet",
        "SPDMGetSignedMeasurements", measurementIndices, nonce, slotId);
}

inline void afterFindObjectForAction(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& componentIntegrityId,
    const std::vector<size_t>& measurementIndices, const std::string& nonce,
    size_t slotId, const boost::system::error_code& ec,
    const dbus::utility::MapperGetSubTreeResponse& subtree)
{
    if (ec.value() == EBADR)
    {
        messages::resourceNotFound(asyncResp->res, "ComponentIntegrity",
                                   componentIntegrityId);
        return;
    }

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS response error {}", ec.value());
        messages::internalError(asyncResp->res);
        return;
    }

    std::optional<std::pair<std::string, std::string>> found =
        findMeasurementSet(subtree, componentIntegrityId);
    if (!found)
    {
        messages::resourceNotFound(asyncResp->res, "ComponentIntegrity",
                                   componentIntegrityId);
        return;
    }
    callSPDMGetSignedMeasurements(asyncResp, found->first, found->second,
                                  measurementIndices, nonce, slotId);
}

// Nonce is a hex-encoded string per the schema.  SPDM nonces are 32 bytes,
// so at most 64 characters, and hex encoding needs whole bytes.  An empty
// nonce is allowed.
constexpr size_t maxNonceSize = 64;

inline bool isHexString(std::string_view str)
{
    return std::ranges::all_of(str, [](char ch) {
        return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
               (ch >= 'A' && ch <= 'F');
    });
}

inline bool validateNonce(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& nonce)
{
    if (!isHexString(nonce) || (nonce.size() % 2) != 0)
    {
        messages::actionParameterValueFormatError(
            asyncResp->res, nonce, "Nonce",
            "ComponentIntegrity.SPDMGetSignedMeasurements");
        return false;
    }
    if (nonce.size() > maxNonceSize)
    {
        messages::stringValueTooLong(asyncResp->res, "Nonce", maxNonceSize);
        return false;
    }
    return true;
}

inline void handleSPDMGetSignedMeasurementsPost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& componentIntegrityId)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    std::optional<std::vector<size_t>> measurementIndices;
    std::optional<std::string> nonce;
    std::optional<size_t> slotId;

    if (!json_util::readJsonAction(req, asyncResp->res, "MeasurementIndices",
                                   measurementIndices, "Nonce", nonce, "SlotId",
                                   slotId))
    {
        return;
    }

    std::vector<size_t> indices;
    if (measurementIndices)
    {
        indices = std::move(*measurementIndices);
    }

    // SPDM devices have at most 255 measurement indices
    constexpr size_t maxIndices = 255;
    if (indices.size() > maxIndices)
    {
        messages::arraySizeTooLong(asyncResp->res, "MeasurementIndices",
                                   maxIndices);
        return;
    }

    std::string nonceStr;
    if (nonce)
    {
        nonceStr = std::move(*nonce);
    }

    if (!validateNonce(asyncResp, nonceStr))
    {
        return;
    }

    size_t slot = 0;
    if (slotId)
    {
        slot = *slotId;
    }

    // SPDM slot IDs are a 3-bit field (0-7)
    if (slot > 7)
    {
        messages::propertyValueOutOfRange(asyncResp->res, std::to_string(slot),
                                          "SlotId");
        return;
    }

    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.Attestation.MeasurementSet"};

    dbus::utility::getSubTree(
        "/xyz/openbmc_project/component_integrity", 0, interfaces,
        std::bind_front(afterFindObjectForAction, asyncResp,
                        componentIntegrityId, indices, nonceStr, slot));
}

inline void requestRoutesComponentIntegrity(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/<str>/")
        .privileges(redfish::privileges::headComponentIntegrity)
        .methods(boost::beast::http::verb::head)(
            std::bind_front(handleComponentIntegrityHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/<str>/")
        .privileges(redfish::privileges::getComponentIntegrity)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleComponentIntegrityGet, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/ComponentIntegrity/<str>/Actions/ComponentIntegrity.SPDMGetSignedMeasurements/")
        .privileges(redfish::privileges::postComponentIntegrity)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleSPDMGetSignedMeasurementsPost, std::ref(app)));
}

inline void requestRoutesComponentIntegrityCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/")
        .privileges(redfish::privileges::headComponentIntegrityCollection)
        .methods(boost::beast::http::verb::head)(std::bind_front(
            handleComponentIntegrityCollectionHead, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/ComponentIntegrity/")
        .privileges(redfish::privileges::getComponentIntegrityCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleComponentIntegrityCollectionGet, std::ref(app)));
}

} // namespace redfish
