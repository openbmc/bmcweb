// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "credential_pipe.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/virtual_media.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/result.hpp>
#include <boost/url/format.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url_view.hpp>
#include <boost/url/url_view_base.hpp>
#include <sdbusplus/message/native_types.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

namespace redfish
{

enum class VmMode
{
    Invalid,
    Legacy,
    Proxy
};

inline VmMode parseObjectPathAndGetMode(
    const sdbusplus::message::object_path& itemPath, const std::string& resName)
{
    std::string thisPath = itemPath.filename();
    BMCWEB_LOG_DEBUG("Filename: {}, ThisPath: {}", itemPath.str, thisPath);

    if (thisPath.empty())
    {
        return VmMode::Invalid;
    }

    if (thisPath != resName)
    {
        return VmMode::Invalid;
    }

    auto mode = itemPath.parent_path();
    auto type = mode.parent_path();

    if (mode.filename().empty() || type.filename().empty())
    {
        return VmMode::Invalid;
    }

    if (type.filename() != "VirtualMedia")
    {
        return VmMode::Invalid;
    }
    std::string modeStr = mode.filename();
    if (modeStr == "Legacy")
    {
        return VmMode::Legacy;
    }
    if (modeStr == "Proxy")
    {
        return VmMode::Proxy;
    }
    return VmMode::Invalid;
}

using CheckItemHandler =
    std::function<void(const std::string& service, const std::string& resName,
                       const std::shared_ptr<bmcweb::AsyncResp>&,
                       const std::pair<sdbusplus::message::object_path,
                                       dbus::utility::DBusInterfacesMap>&)>;

inline void findAndParseObject(
    const std::string& service, const std::string& resName,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    CheckItemHandler&& handler)
{
    sdbusplus::message::object_path path("/xyz/openbmc_project/VirtualMedia");
    dbus::utility::getManagedObjects(
        service, path,
        [service, resName, asyncResp, handler = std::move(handler)](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");

                return;
            }

            for (const auto& item : subtree)
            {
                VmMode mode = parseObjectPathAndGetMode(item.first, resName);
                if (mode != VmMode::Invalid)
                {
                    handler(service, resName, asyncResp, item);
                    return;
                }
            }

            BMCWEB_LOG_DEBUG("Parent item not found");
            asyncResp->res.result(boost::beast::http::status::not_found);
        });
}

/**
 * @brief Function extracts transfer protocol name from URI.
 */
inline std::string getTransferProtocolTypeFromUri(const std::string& imageUri)
{
    boost::system::result<boost::urls::url_view> url =
        boost::urls::parse_uri(imageUri);
    if (!url)
    {
        return "None";
    }
    std::string_view scheme = url->scheme();
    if (scheme == "smb")
    {
        return "CIFS";
    }
    if (scheme == "https")
    {
        return "HTTPS";
    }

    return "None";
}

/**
 * @brief Read all known properties from VM object interfaces
 */
inline void vmParseInterfaceObject(
    const dbus::utility::DBusInterfacesMap& interfaces,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    for (const auto& [interface, values] : interfaces)
    {
        if (interface == "xyz.openbmc_project.VirtualMedia.MountPoint")
        {
            for (const auto& [property, value] : values)
            {
                if (property == "EndpointId")
                {
                    const std::string* endpointIdValue =
                        std::get_if<std::string>(&value);
                    if (endpointIdValue == nullptr)
                    {
                        continue;
                    }
                    if (!endpointIdValue->empty())
                    {
                        // Proxy mode
                        asyncResp->res
                            .jsonValue["Oem"]["OpenBMC"]["WebSocketEndpoint"] =
                            *endpointIdValue;
                        asyncResp->res.jsonValue["TransferProtocolType"] =
                            "OEM";
                    }
                }
                if (property == "ImageURL")
                {
                    const std::string* imageUrlValue =
                        std::get_if<std::string>(&value);
                    if (imageUrlValue != nullptr && !imageUrlValue->empty())
                    {
                        std::filesystem::path filePath = *imageUrlValue;
                        if (!filePath.has_filename())
                        {
                            // this will handle https share, which not
                            // necessarily has to have filename given.
                            asyncResp->res.jsonValue["ImageName"] = "";
                        }
                        else
                        {
                            asyncResp->res.jsonValue["ImageName"] =
                                filePath.filename();
                        }

                        asyncResp->res.jsonValue["Image"] = *imageUrlValue;
                        asyncResp->res.jsonValue["TransferProtocolType"] =
                            getTransferProtocolTypeFromUri(*imageUrlValue);

                        asyncResp->res.jsonValue["ConnectedVia"] =
                            virtual_media::ConnectedVia::URI;
                    }
                }
                if (property == "WriteProtected")
                {
                    const bool* writeProtectedValue = std::get_if<bool>(&value);
                    if (writeProtectedValue != nullptr)
                    {
                        asyncResp->res.jsonValue["WriteProtected"] =
                            *writeProtectedValue;
                    }
                }
            }
        }
        if (interface == "xyz.openbmc_project.VirtualMedia.Process")
        {
            for (const auto& [property, value] : values)
            {
                if (property == "Active")
                {
                    const bool* activeValue = std::get_if<bool>(&value);
                    if (activeValue == nullptr)
                    {
                        BMCWEB_LOG_DEBUG("Value Active not found");
                        return;
                    }
                    asyncResp->res.jsonValue["Inserted"] = *activeValue;

                    if (*activeValue)
                    {
                        asyncResp->res.jsonValue["ConnectedVia"] =
                            virtual_media::ConnectedVia::Applet;
                    }
                }
            }
        }
    }
}

/**
 * @brief Fill template for Virtual Media Item.
 */
inline nlohmann::json vmItemTemplate(const std::string& name,
                                     const std::string& resName)
{
    nlohmann::json item;
    item["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/VirtualMedia/{}", name, resName);

    item["@odata.type"] = "#VirtualMedia.v1_3_0.VirtualMedia";
    item["Name"] = "Virtual Removable Media";
    item["Id"] = resName;
    item["WriteProtected"] = true;
    item["ConnectedVia"] = virtual_media::ConnectedVia::NotConnected;
    item["MediaTypes"] = nlohmann::json::array_t({"CD", "USBStick"});
    item["TransferMethod"] = virtual_media::TransferMethod::Stream;
    item["Oem"]["OpenBMC"]["@odata.type"] =
        "#OpenBMCVirtualMedia.v1_0_0.VirtualMedia";
    item["Oem"]["OpenBMC"]["@odata.id"] = boost::urls::format(
        "/redfish/v1/Managers/{}/VirtualMedia/{}#/Oem/OpenBMC", name, resName);

    return item;
}

/**
 *  @brief Fills collection data
 */
inline void getVmResourceList(std::shared_ptr<bmcweb::AsyncResp> asyncResp,
                              const std::string& service,
                              const std::string& name)
{
    BMCWEB_LOG_DEBUG("Get available Virtual Media resources.");
    sdbusplus::message::object_path objPath(
        "/xyz/openbmc_project/VirtualMedia");
    dbus::utility::getManagedObjects(
        service, objPath,
        [name, asyncResp{std::move(asyncResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("DBUS response error");
                return;
            }
            nlohmann::json& members = asyncResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto& object : subtree)
            {
                nlohmann::json item;
                std::string path = object.first.filename();
                if (path.empty())
                {
                    continue;
                }

                item["@odata.id"] = boost::urls::format(
                    "/redfish/v1/Managers/{}/VirtualMedia/{}", name, path);
                members.emplace_back(std::move(item));
            }
            asyncResp->res.jsonValue["Members@odata.count"] = members.size();
        });
}

inline void afterGetVmData(
    const std::string& name, const std::string& /*service*/,
    const std::string& resName,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::pair<sdbusplus::message::object_path,
                    dbus::utility::DBusInterfacesMap>& item)
{
    VmMode mode = parseObjectPathAndGetMode(item.first, resName);
    if (mode == VmMode::Invalid)
    {
        return;
    }

    asyncResp->res.jsonValue = vmItemTemplate(name, resName);

    // Check if dbus path is Legacy type
    if (mode == VmMode::Legacy)
    {
        asyncResp->res.jsonValue["Actions"]["#VirtualMedia.InsertMedia"]
                                ["target"] = boost::urls::format(
            "/redfish/v1/Managers/{}/VirtualMedia/{}/Actions/VirtualMedia.InsertMedia",
            name, resName);
    }

    vmParseInterfaceObject(item.second, asyncResp);

    asyncResp->res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"]
                            ["target"] = boost::urls::format(
        "/redfish/v1/Managers/{}/VirtualMedia/{}/Actions/VirtualMedia.EjectMedia",
        name, resName);
}

/**
 *  @brief Fills data for specific resource
 */
inline void getVmData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& service, const std::string& name,
                      const std::string& resName)
{
    BMCWEB_LOG_DEBUG("Get Virtual Media resource data.");

    findAndParseObject(service, resName, asyncResp,
                       std::bind_front(afterGetVmData, name));
}

/**
 * @brief Transfer protocols supported for InsertMedia action.
 *
 */
enum class TransferProtocol
{
    https,
    smb,
    invalid
};

/**
 * @brief Function extracts transfer protocol type from URI.
 *
 */
inline std::optional<TransferProtocol> getTransferProtocolFromUri(
    const boost::urls::url_view_base& imageUri)
{
    std::string_view scheme = imageUri.scheme();
    if (scheme == "smb")
    {
        return TransferProtocol::smb;
    }
    if (scheme == "https")
    {
        return TransferProtocol::https;
    }
    if (!scheme.empty())
    {
        return TransferProtocol::invalid;
    }

    return {};
}

/**
 * @brief Function convert transfer protocol from string param.
 *
 */
inline std::optional<TransferProtocol> getTransferProtocolFromParam(
    const std::optional<std::string>& transferProtocolType)
{
    if (!transferProtocolType)
    {
        return {};
    }

    if (*transferProtocolType == "CIFS")
    {
        return TransferProtocol::smb;
    }

    if (*transferProtocolType == "HTTPS")
    {
        return TransferProtocol::https;
    }

    return TransferProtocol::invalid;
}

/**
 * @brief Function extends URI with transfer protocol type.
 *
 */
inline std::string getUriWithTransferProtocol(
    const std::string& imageUri, const TransferProtocol& transferProtocol)
{
    if (transferProtocol == TransferProtocol::smb)
    {
        return "smb://" + imageUri;
    }

    if (transferProtocol == TransferProtocol::https)
    {
        return "https://" + imageUri;
    }

    return imageUri;
}

struct InsertMediaActionParams
{
    std::optional<std::string> imageUrl;
    std::optional<std::string> userName;
    std::optional<std::string> password;
    std::optional<std::string> transferMethod;
    std::optional<std::string> transferProtocolType;
    std::optional<bool> writeProtected = true;
    std::optional<bool> inserted;
};

/**
 * @brief Function transceives data with dbus directly.
 *
 * All BMC state properties will be retrieved before sending reset request.
 */
inline void doMountVmLegacy(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& service, const std::string& name,
                            const std::string& imageUrl, bool rw,
                            std::string&& userName, std::string&& password)
{
    if (userName.contains('\0'))
    {
        messages::actionParameterValueFormatError(
            asyncResp->res, userName, "Username", "VirtualMedia.InsertMedia");
        return;
    }
    constexpr uint64_t limit = 512;
    if (userName.size() > limit)
    {
        messages::stringValueTooLong(asyncResp->res, userName, limit);
        return;
    }
    if (password.contains('\0'))
    {
        messages::actionParameterValueFormatError(
            asyncResp->res, password, "Password", "VirtualMedia.InsertMedia");
        return;
    }
    if (password.size() > limit)
    {
        messages::stringValueTooLong(asyncResp->res, password, limit);
        return;
    }
    // Open pipe
    std::shared_ptr<CredentialsPipe> secretPipe =
        std::make_shared<CredentialsPipe>(
            crow::connections::systemBus->get_io_context());
    int fd = secretPipe->releaseFd();

    // Pass secret over pipe
    secretPipe->asyncWrite(
        std::move(userName), std::move(password),
        [asyncResp,
         secretPipe](const boost::system::error_code& ec, std::size_t) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to pass secret: {}", ec);
                messages::internalError(asyncResp->res);
            }
        });

    sdbusplus::message::unix_fd unixFd(fd);

    sdbusplus::message::object_path path(
        "/xyz/openbmc_project/VirtualMedia/Legacy");
    path /= name;
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code& ec, bool success) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Bad D-Bus request error: {}", ec);
                messages::internalError(asyncResp->res);
                return;
            }
            if (!success)
            {
                BMCWEB_LOG_ERROR("Service responded with error");
                messages::internalError(asyncResp->res);
            }
        },
        service, path.str, "xyz.openbmc_project.VirtualMedia.Legacy", "Mount",
        imageUrl, rw, unixFd);
}

/**
 * @brief Function validate parameters of insert media request.
 *
 */
inline void validateParams(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& service,
                           const std::string& resName,
                           InsertMediaActionParams& actionParams)
{
    BMCWEB_LOG_DEBUG("Validation started");
    // required param imageUrl must not be empty
    if (!actionParams.imageUrl)
    {
        BMCWEB_LOG_ERROR("Request action parameter Image is empty.");

        messages::propertyValueFormatError(asyncResp->res, "<empty>", "Image");

        return;
    }

    // optional param inserted must be true
    if (actionParams.inserted && !*actionParams.inserted)
    {
        BMCWEB_LOG_ERROR(
            "Request action optional parameter Inserted must be true.");

        messages::actionParameterNotSupported(asyncResp->res, "Inserted",
                                              "InsertMedia");

        return;
    }

    // optional param transferMethod must be stream
    if (actionParams.transferMethod &&
        (*actionParams.transferMethod != "Stream"))
    {
        BMCWEB_LOG_ERROR("Request action optional parameter "
                         "TransferMethod must be Stream.");

        messages::actionParameterNotSupported(asyncResp->res, "TransferMethod",
                                              "InsertMedia");

        return;
    }
    boost::system::result<boost::urls::url_view> url =
        boost::urls::parse_uri(*actionParams.imageUrl);
    if (!url)
    {
        messages::actionParameterValueFormatError(
            asyncResp->res, *actionParams.imageUrl, "Image", "InsertMedia");
        return;
    }
    std::optional<TransferProtocol> uriTransferProtocolType =
        getTransferProtocolFromUri(*url);

    std::optional<TransferProtocol> paramTransferProtocolType =
        getTransferProtocolFromParam(actionParams.transferProtocolType);

    // ImageUrl does not contain valid protocol type
    if (uriTransferProtocolType &&
        *uriTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR("Request action parameter ImageUrl must "
                         "contain specified protocol type from list: "
                         "(smb, https).");

        messages::resourceAtUriInUnknownFormat(asyncResp->res, *url);

        return;
    }

    // transferProtocolType should contain value from list
    if (paramTransferProtocolType &&
        *paramTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR("Request action parameter TransferProtocolType "
                         "must be provided with value from list: "
                         "(CIFS, HTTPS).");

        messages::propertyValueNotInList(
            asyncResp->res, actionParams.transferProtocolType.value_or(""),
            "TransferProtocolType");
        return;
    }

    // valid transfer protocol not provided either with URI nor param
    if (!uriTransferProtocolType && !paramTransferProtocolType)
    {
        BMCWEB_LOG_ERROR("Request action parameter ImageUrl must "
                         "contain specified protocol type or param "
                         "TransferProtocolType must be provided.");

        messages::resourceAtUriInUnknownFormat(asyncResp->res, *url);

        return;
    }

    // valid transfer protocol provided both with URI and param
    if (paramTransferProtocolType && uriTransferProtocolType)
    {
        // check if protocol is the same for URI and param
        if (*paramTransferProtocolType != *uriTransferProtocolType)
        {
            BMCWEB_LOG_ERROR("Request action parameter "
                             "TransferProtocolType must  contain the "
                             "same protocol type as protocol type "
                             "provided with param imageUrl.");

            messages::actionParameterValueTypeError(
                asyncResp->res, actionParams.transferProtocolType.value_or(""),
                "TransferProtocolType", "InsertMedia");

            return;
        }
    }

    // validation passed, add protocol to URI if needed
    if (!uriTransferProtocolType && paramTransferProtocolType)
    {
        actionParams.imageUrl = getUriWithTransferProtocol(
            *actionParams.imageUrl, *paramTransferProtocolType);
    }

    if (!actionParams.userName)
    {
        actionParams.userName = "";
    }

    if (!actionParams.password)
    {
        actionParams.password = "";
    }

    doMountVmLegacy(asyncResp, service, resName, *actionParams.imageUrl,
                    !(actionParams.writeProtected.value_or(false)),
                    std::move(*actionParams.userName),
                    std::move(*actionParams.password));
}

/**
 * @brief Function transceives data with dbus directly.
 *
 * All BMC state properties will be retrieved before sending reset request.
 */
inline void doEjectAction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& service, const std::string& name,
                          bool legacy)
{
    // Legacy mount requires parameter with image
    if (legacy)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Bad D-Bus request error: {}", ec);

                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            service, "/xyz/openbmc_project/VirtualMedia/Legacy/" + name,
            "xyz.openbmc_project.VirtualMedia.Legacy", "Unmount");
    }
    else // proxy
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code& ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR("Bad D-Bus request error: {}", ec);

                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            service, "/xyz/openbmc_project/VirtualMedia/Proxy/" + name,
            "xyz.openbmc_project.VirtualMedia.Proxy", "Unmount");
    }
}

inline void handleManagersVirtualMediaActionInsertPost(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& name, const std::string& resName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    constexpr std::string_view action = "VirtualMedia.InsertMedia";
    if (name != "bmc")
    {
        messages::resourceNotFound(asyncResp->res, action, resName);

        return;
    }
    InsertMediaActionParams actionParams;

    // Read obligatory parameters (url of image)
    if (!json_util::readJsonAction(                                    //
            req, asyncResp->res,                                       //
            "Image", actionParams.imageUrl,                            //
            "Inserted", actionParams.inserted,                         //
            "Password", actionParams.password,                         //
            "TransferMethod", actionParams.transferMethod,             //
            "TransferProtocolType", actionParams.transferProtocolType, //
            "UserName", actionParams.userName,                         //
            "WriteProtected", actionParams.writeProtected              //
            ))
    {
        return;
    }

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/VirtualMedia", {},
        [asyncResp, action, actionParams,
         resName](const boost::system::error_code& ec,
                  const dbus::utility::MapperGetObject& getObjectType) mutable {
            if (ec)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}", ec);
                messages::resourceNotFound(asyncResp->res, action, resName);

                return;
            }

            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG("GetObjectType: {}", service);

            sdbusplus::message::object_path path(
                "/xyz/openbmc_project/VirtualMedia");
            dbus::utility::getManagedObjects(
                service, path,
                [service, resName, action, actionParams, asyncResp](
                    const boost::system::error_code& ec2,
                    const dbus::utility::ManagedObjectType& subtree) mutable {
                    if (ec2)
                    {
                        // Not possible in proxy mode
                        BMCWEB_LOG_DEBUG("InsertMedia not "
                                         "allowed in proxy mode");
                        messages::resourceNotFound(asyncResp->res, action,
                                                   resName);

                        return;
                    }
                    for (const auto& object : subtree)
                    {
                        VmMode mode =
                            parseObjectPathAndGetMode(object.first, resName);
                        if (mode == VmMode::Legacy)
                        {
                            validateParams(asyncResp, service, resName,
                                           actionParams);

                            return;
                        }
                    }
                    BMCWEB_LOG_DEBUG("Parent item not found");
                    messages::resourceNotFound(asyncResp->res, "VirtualMedia",
                                               resName);
                });
        });
}

inline void handleManagersVirtualMediaActionEject(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& managerName, const std::string& resName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    constexpr std::string_view action = "VirtualMedia.EjectMedia";
    if (managerName != "bmc")
    {
        messages::resourceNotFound(asyncResp->res, action, resName);

        return;
    }

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/VirtualMedia", {},
        [asyncResp, action,
         resName](const boost::system::error_code& ec2,
                  const dbus::utility::MapperGetObject& getObjectType) {
            if (ec2)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}",
                                 ec2);
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG("GetObjectType: {}", service);

            sdbusplus::message::object_path path(
                "/xyz/openbmc_project/VirtualMedia");
            dbus::utility::getManagedObjects(
                service, path,
                [resName, service, action,
                 asyncResp](const boost::system::error_code& ec,
                            const dbus::utility::ManagedObjectType& subtree) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR("ObjectMapper : No Service found");
                        messages::resourceNotFound(asyncResp->res, action,
                                                   resName);
                        return;
                    }

                    for (const auto& object : subtree)
                    {
                        VmMode mode =
                            parseObjectPathAndGetMode(object.first, resName);
                        if (mode != VmMode::Invalid)
                        {
                            doEjectAction(asyncResp, service, resName,
                                          mode == VmMode::Legacy);
                            return;
                        }
                    }
                    BMCWEB_LOG_DEBUG("Parent item not found");
                    messages::resourceNotFound(asyncResp->res, "VirtualMedia",
                                               resName);
                });
        });
}

inline void handleManagersVirtualMediaCollectionGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& name)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (name != "bmc")
    {
        messages::resourceNotFound(asyncResp->res, "VirtualMedia", name);

        return;
    }

    asyncResp->res.jsonValue["@odata.type"] =
        "#VirtualMediaCollection.VirtualMediaCollection";
    asyncResp->res.jsonValue["Name"] = "Virtual Media Services";
    asyncResp->res.jsonValue["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/VirtualMedia", name);

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/VirtualMedia", {},
        [asyncResp, name](const boost::system::error_code& ec,
                          const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}", ec);
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG("GetObjectType: {}", service);

            getVmResourceList(asyncResp, service, name);
        });
}

inline void handleVirtualMediaGet(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& name, const std::string& resName)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    if (name != "bmc")
    {
        messages::resourceNotFound(asyncResp->res, "VirtualMedia", resName);

        return;
    }

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/VirtualMedia", {},
        [asyncResp, name,
         resName](const boost::system::error_code& ec,
                  const dbus::utility::MapperGetObject& getObjectType) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("ObjectMapper::GetObject call failed: {}", ec);
                messages::internalError(asyncResp->res);

                return;
            }
            std::string service = getObjectType.begin()->first;
            BMCWEB_LOG_DEBUG("GetObjectType: {}", service);

            getVmData(asyncResp, service, name, resName);
        });
}

inline void requestNBDVirtualMediaRoutes(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.InsertMedia")
        .privileges(redfish::privileges::postVirtualMedia)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleManagersVirtualMediaActionInsertPost, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.EjectMedia")
        .privileges(redfish::privileges::postVirtualMedia)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            handleManagersVirtualMediaActionEject, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/VirtualMedia/")
        .privileges(redfish::privileges::getVirtualMediaCollection)
        .methods(boost::beast::http::verb::get)(std::bind_front(
            handleManagersVirtualMediaCollectionGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/VirtualMedia/<str>/")
        .privileges(redfish::privileges::getVirtualMedia)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleVirtualMediaGet, std::ref(app)));
}

} // namespace redfish
