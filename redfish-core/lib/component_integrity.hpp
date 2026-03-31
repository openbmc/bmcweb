// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
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

inline std::optional<std::string> dbusTypeToRedfishType(
    const std::string& dbusType)
{
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.SPDM")
    {
        return "SPDM";
    }
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.TPM")
    {
        return "TPM";
    }
    if (dbusType ==
        "xyz.openbmc_project.Attestation.ComponentIntegrity.SecurityTechnologyType.OEM")
    {
        return "OEM";
    }
    return std::nullopt;
}

inline std::optional<std::string> dbusVerificationStatusToRedfish(
    const std::string& dbusStatus)
{
    if (dbusStatus ==
        "xyz.openbmc_project.Attestation.IdentityAuthentication.VerificationStatus.Success")
    {
        return "Success";
    }
    if (dbusStatus ==
        "xyz.openbmc_project.Attestation.IdentityAuthentication.VerificationStatus.Failed")
    {
        return "Failed";
    }
    return std::nullopt;
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
        std::optional<std::string> rfType = dbusTypeToRedfishType(*type);
        if (rfType)
        {
            asyncResp->res.jsonValue["ComponentIntegrityType"] = *rfType;
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
        std::optional<std::string> rfStatus =
            dbusVerificationStatusToRedfish(*responderStatus);
        if (rfStatus)
        {
            asyncResp->res
                .jsonValue["SPDM"]["IdentityAuthentication"]
                          ["ResponderAuthentication"]["VerificationStatus"] =
                *rfStatus;
        }
    }
}

inline void fillTargetComponentURI(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec,
    const dbus::utility::MapperEndPoints& endpoints)
{
    if (ec || endpoints.empty())
    {
        // No association is not an error
        return;
    }

    // Map the D-Bus inventory path to the corresponding Redfish URI.
    sdbusplus::object_path targetPath(endpoints[0]);
    std::string targetId = targetPath.filename();
    if (targetId.empty())
    {
        return;
    }

    std::string parentPath = targetPath.parent_path();
    boost::urls::url targetURI;
    if (parentPath == "/xyz/openbmc_project/inventory/system/chassis" ||
        parentPath == "/xyz/openbmc_project/inventory/chassis")
    {
        targetURI = boost::urls::format("/redfish/v1/Chassis/{}", targetId);
    }
    else if (parentPath == "/xyz/openbmc_project/inventory/system/board" ||
             parentPath == "/xyz/openbmc_project/inventory/system")
    {
        targetURI = boost::urls::format("/redfish/v1/Systems/{}", targetId);
    }
    else
    {
        // Unknown inventory location — omit rather than leak D-Bus path
        BMCWEB_LOG_WARNING("Cannot map inventory path {} to Redfish URI",
                           endpoints[0]);
        return;
    }
    asyncResp->res.jsonValue["TargetComponentURI"] = targetURI;
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

        // TargetComponentURI is required by the schema. If the D-Bus
        // "authenticating" association resolves to a known inventory path,
        // fillTargetComponentURI will override this with the proper
        // Chassis/System URI. Otherwise, default to the Chassis collection
        // since the attested device is a component on the platform.
        asyncResp->res.jsonValue["TargetComponentURI"] = "/redfish/v1/Chassis";

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

    for (const auto& [objectPath, serviceMap] : subtree)
    {
        sdbusplus::object_path path(objectPath);
        if (path.filename() != componentIntegrityId)
        {
            continue;
        }

        for (const auto& [service, interfaces] : serviceMap)
        {
            for (const auto& iface : interfaces)
            {
                if (iface != "xyz.openbmc_project.Attestation.MeasurementSet")
                {
                    continue;
                }

                dbus::utility::async_method_call(
                    [asyncResp](const boost::system::error_code& ec2,
                                const sdbusplus::object_path& certificate,
                                const std::string& hashingAlgorithm,
                                const std::string& publicKey,
                                const std::string& signedMeasurements,
                                const std::string& signingAlgorithm,
                                const std::string& version) {
                        if (ec2)
                        {
                            BMCWEB_LOG_ERROR(
                                "SPDMGetSignedMeasurements D-Bus error: {}",
                                ec2);
                            messages::internalError(asyncResp->res);
                            return;
                        }

                        // Map certificate D-Bus path to Redfish URI
                        sdbusplus::object_path certPath(certificate.str);
                        std::string certId = certPath.filename();
                        std::string certParent = certPath.parent_path();
                        if (certParent ==
                            "/xyz/openbmc_project/certs/server/https")
                        {
                            asyncResp->res
                                .jsonValue["Certificate"]
                                          ["@odata.id"] = boost::urls::format(
                                "/redfish/v1/Managers/{}/NetworkProtocol/HTTPS/Certificates/{}",
                                BMCWEB_REDFISH_MANAGER_URI_NAME, certId);
                        }
                        else if (certParent ==
                                 "/xyz/openbmc_project/certs/client/ldap")
                        {
                            asyncResp->res
                                .jsonValue["Certificate"]
                                          ["@odata.id"] = boost::urls::format(
                                "/redfish/v1/AccountService/LDAP/Certificates/{}",
                                certId);
                        }
                        else if (
                            certParent ==
                            "/xyz/openbmc_project/certs/authority/truststore")
                        {
                            asyncResp->res
                                .jsonValue["Certificate"]
                                          ["@odata.id"] = boost::urls::format(
                                "/redfish/v1/Managers/{}/Truststore/Certificates/{}",
                                BMCWEB_REDFISH_MANAGER_URI_NAME, certId);
                        }
                        else
                        {
                            BMCWEB_LOG_WARNING(
                                "Cannot map certificate path {} to Redfish URI",
                                certificate.str);
                        }
                        asyncResp->res.jsonValue["HashingAlgorithm"] =
                            hashingAlgorithm;
                        asyncResp->res.jsonValue["PublicKey"] = publicKey;
                        asyncResp->res.jsonValue["SignedMeasurements"] =
                            signedMeasurements;
                        asyncResp->res.jsonValue["SigningAlgorithm"] =
                            signingAlgorithm;
                        asyncResp->res.jsonValue["Version"] = version;
                    },
                    service, objectPath,
                    "xyz.openbmc_project.Attestation.MeasurementSet",
                    "SPDMGetSignedMeasurements", measurementIndices, nonce,
                    slotId);
                return;
            }
        }
    }

    messages::resourceNotFound(asyncResp->res, "ComponentIntegrity",
                               componentIntegrityId);
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

    // Nonce must be a hex-encoded string per the schema. all_of() on an empty
    // string is true, so an absent/empty nonce is still allowed. This also
    // rejects embedded null bytes that would otherwise truncate error messages.
    if (!std::ranges::all_of(nonceStr, [](char ch) {
            return (ch >= '0' && ch <= '9') || (ch >= 'a' && ch <= 'f') ||
                   (ch >= 'A' && ch <= 'F');
        }))
    {
        messages::actionParameterValueFormatError(
            asyncResp->res, nonceStr, "Nonce",
            "ComponentIntegrity.SPDMGetSignedMeasurements");
        return;
    }
    // SPDM nonces are 32 bytes; allow up to 64 chars for hex encoding
    constexpr size_t maxNonceSize = 64;
    if (nonceStr.size() > maxNonceSize)
    {
        messages::stringValueTooLong(asyncResp->res, "Nonce", maxNonceSize);
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
