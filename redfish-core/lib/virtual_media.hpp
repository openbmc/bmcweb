/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "account_service.hpp"
#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "generated/enums/virtual_media.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/json_utils.hpp"

#include <boost/process/async_pipe.hpp>
#include <boost/url/url_view.hpp>

#include <array>
#include <string_view>

namespace redfish
{

enum class VmMode
{
    Invalid,
    Legacy,
    Proxy
};

inline VmMode
    parseObjectPathAndGetMode(const sdbusplus::message::object_path& itemPath,
                              const std::string& resName)
{
    std::string thisPath = itemPath.filename();
    BMCWEB_LOG_DEBUG << "Filename: " << itemPath.str
                     << ", ThisPath: " << thisPath;

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
                       std::pair<sdbusplus::message::object_path,
                                 dbus::utility::DBusInteracesMap>&)>;

inline void findAndParseObject(const std::string& service,
                               const std::string& resName,
                               const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               CheckItemHandler&& handler)
{
    crow::connections::systemBus->async_method_call(
        [service, resName, aResp,
         handler](const boost::system::error_code ec,
                  dbus::utility::ManagedObjectType& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";

            return;
        }

        for (auto& item : subtree)
        {
            VmMode mode = parseObjectPathAndGetMode(item.first, resName);
            if (mode != VmMode::Invalid)
            {
                handler(service, resName, aResp, item);
                return;
            }
        }

        BMCWEB_LOG_DEBUG << "Parent item not found";
        aResp->res.result(boost::beast::http::status::not_found);
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 * @brief Function extracts transfer protocol name from URI.
 */
inline std::string getTransferProtocolTypeFromUri(const std::string& imageUri)
{
    boost::urls::result<boost::urls::url_view> url =
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
inline void
    vmParseInterfaceObject(const dbus::utility::DBusInteracesMap& interfaces,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp)
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
                        aResp->res
                            .jsonValue["Oem"]["OpenBMC"]["WebSocketEndpoint"] =
                            *endpointIdValue;
                        aResp->res.jsonValue["TransferProtocolType"] = "OEM";
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
                            aResp->res.jsonValue["ImageName"] = "";
                        }
                        else
                        {
                            aResp->res.jsonValue["ImageName"] =
                                filePath.filename();
                        }

                        aResp->res.jsonValue["Image"] = *imageUrlValue;
                        aResp->res.jsonValue["TransferProtocolType"] =
                            getTransferProtocolTypeFromUri(*imageUrlValue);

                        aResp->res.jsonValue["ConnectedVia"] =
                            virtual_media::ConnectedVia::URI;
                    }
                }
                if (property == "WriteProtected")
                {
                    const bool* writeProtectedValue = std::get_if<bool>(&value);
                    if (writeProtectedValue != nullptr)
                    {
                        aResp->res.jsonValue["WriteProtected"] =
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
                        BMCWEB_LOG_DEBUG << "Value Active not found";
                        return;
                    }
                    aResp->res.jsonValue["Inserted"] = *activeValue;

                    if (*activeValue)
                    {
                        aResp->res.jsonValue["ConnectedVia"] =
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
    item["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Managers", name, "VirtualMedia", resName);

    item["@odata.type"] = "#VirtualMedia.v1_3_0.VirtualMedia";
    item["Name"] = "Virtual Removable Media";
    item["Id"] = resName;
    item["WriteProtected"] = true;
    item["ConnectedVia"] = virtual_media::ConnectedVia::NotConnected;
    item["MediaTypes"] = nlohmann::json::array_t({"CD", "USBStick"});
    item["TransferMethod"] = "Stream";
    item["Oem"]["OpenBMC"]["@odata.type"] =
        "#OemVirtualMedia.v1_0_0.VirtualMedia";

    return item;
}

/**
 *  @brief Fills collection data
 */
inline void getVmResourceList(std::shared_ptr<bmcweb::AsyncResp> aResp,
                              const std::string& service,
                              const std::string& name)
{
    BMCWEB_LOG_DEBUG << "Get available Virtual Media resources.";
    crow::connections::systemBus->async_method_call(
        [name, aResp{std::move(aResp)}](
            const boost::system::error_code& ec,
            const dbus::utility::ManagedObjectType& subtree) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "DBUS response error";
            return;
        }
        nlohmann::json& members = aResp->res.jsonValue["Members"];
        members = nlohmann::json::array();

        for (const auto& object : subtree)
        {
            nlohmann::json item;
            std::string path = object.first.filename();
            if (path.empty())
            {
                continue;
            }

            item["@odata.id"] = crow::utility::urlFromPieces(
                "redfish", "v1", "Managers", name, "VirtualMedia", path);
            members.emplace_back(std::move(item));
        }
        aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void afterGetVmData(const std::string& name,
                           const std::string& /*service*/,
                           const std::string& resName,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           std::pair<sdbusplus::message::object_path,
                                     dbus::utility::DBusInteracesMap>& item)
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
        asyncResp->res
            .jsonValue["Actions"]["#VirtualMedia.InsertMedia"]["target"] =
            crow::utility::urlFromPieces("redfish", "v1", "Managers", name,
                                         "VirtualMedia", resName, "Actions",
                                         "VirtualMedia.InsertMedia");
    }

    vmParseInterfaceObject(item.second, asyncResp);

    asyncResp->res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"]["target"] =
        crow::utility::urlFromPieces("redfish", "v1", "Managers", name,
                                     "VirtualMedia", resName, "Actions",
                                     "VirtualMedia.EjectMedia");
}

/**
 *  @brief Fills data for specific resource
 */
inline void getVmData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                      const std::string& service, const std::string& name,
                      const std::string& resName)
{
    BMCWEB_LOG_DEBUG << "Get Virtual Media resource data.";

    findAndParseObject(service, resName, aResp,
                       std::bind_front(&afterGetVmData, name));
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
inline std::optional<TransferProtocol>
    getTransferProtocolFromUri(boost::urls::url_view imageUri)
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
    if (transferProtocolType == std::nullopt)
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
inline std::string
    getUriWithTransferProtocol(const std::string& imageUri,
                               const TransferProtocol& transferProtocol)
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

template <typename T>
static void secureCleanup(T& value)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
    auto raw = const_cast<typename T::value_type*>(value.data());
    explicit_bzero(raw, value.size() * sizeof(*raw));
}

class Credentials
{
  public:
    Credentials(std::string&& user, std::string&& password) :
        userBuf(std::move(user)), passBuf(std::move(password))
    {}

    ~Credentials()
    {
        secureCleanup(userBuf);
        secureCleanup(passBuf);
    }

    const std::string& user()
    {
        return userBuf;
    }

    const std::string& password()
    {
        return passBuf;
    }

    Credentials() = delete;
    Credentials(const Credentials&) = delete;
    Credentials& operator=(const Credentials&) = delete;
    Credentials(Credentials&&) = delete;
    Credentials& operator=(Credentials&&) = delete;

  private:
    std::string userBuf;
    std::string passBuf;
};

class CredentialsProvider
{
  public:
    template <typename T>
    struct Deleter
    {
        void operator()(T* buff) const
        {
            if (buff)
            {
                secureCleanup(*buff);
                delete buff;
            }
        }
    };

    using Buffer = std::vector<char>;
    using SecureBuffer = std::unique_ptr<Buffer, Deleter<Buffer>>;
    // Using explicit definition instead of std::function to avoid implicit
    // conversions eg. stack copy instead of reference
    using FormatterFunc = void(const std::string& username,
                               const std::string& password, Buffer& dest);

    CredentialsProvider(std::string&& user, std::string&& password) :
        credentials(std::move(user), std::move(password))
    {}

    const std::string& user()
    {
        return credentials.user();
    }

    const std::string& password()
    {
        return credentials.password();
    }

    SecureBuffer pack(FormatterFunc* formatter)
    {
        SecureBuffer packed{new Buffer{}};
        if (formatter != nullptr)
        {
            formatter(credentials.user(), credentials.password(), *packed);
        }

        return packed;
    }

  private:
    Credentials credentials;
};

// Wrapper for boost::async_pipe ensuring proper pipe cleanup
class SecurePipe
{
  public:
    using unix_fd = sdbusplus::message::unix_fd;

    SecurePipe(boost::asio::io_context& io,
               CredentialsProvider::SecureBuffer&& bufferIn) :
        impl(io),
        buffer{std::move(bufferIn)}
    {}

    ~SecurePipe()
    {
        // Named pipe needs to be explicitly removed
        impl.close();
    }

    SecurePipe(const SecurePipe&) = delete;
    SecurePipe(SecurePipe&&) = delete;
    SecurePipe& operator=(const SecurePipe&) = delete;
    SecurePipe& operator=(SecurePipe&&) = delete;

    unix_fd fd() const
    {
        return unix_fd{impl.native_source()};
    }

    template <typename WriteHandler>
    void asyncWrite(WriteHandler&& handler)
    {
        impl.async_write_some(boost::asio::buffer(*buffer),
                              std::forward<WriteHandler>(handler));
    }

    const std::string name;
    boost::process::async_pipe impl;
    CredentialsProvider::SecureBuffer buffer;
};

/**
 * @brief Function transceives data with dbus directly.
 *
 * All BMC state properties will be retrieved before sending reset request.
 */
inline void doMountVmLegacy(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& service, const std::string& name,
                            const std::string& imageUrl, const bool rw,
                            std::string&& userName, std::string&& password)
{
    constexpr const size_t secretLimit = 1024;

    std::shared_ptr<SecurePipe> secretPipe;
    dbus::utility::DbusVariantType unixFd = -1;

    if (!userName.empty() || !password.empty())
    {
        // Encapsulate in safe buffer
        CredentialsProvider credentials(std::move(userName),
                                        std::move(password));

        // Payload must contain data + NULL delimiters
        if (credentials.user().size() + credentials.password().size() + 2 >
            secretLimit)
        {
            BMCWEB_LOG_ERROR << "Credentials too long to handle";
            messages::unrecognizedRequestBody(asyncResp->res);
            return;
        }

        // Pack secret
        auto secret = credentials.pack(
            [](const auto& user, const auto& pass, auto& buff) {
            std::copy(user.begin(), user.end(), std::back_inserter(buff));
            buff.push_back('\0');
            std::copy(pass.begin(), pass.end(), std::back_inserter(buff));
            buff.push_back('\0');
        });

        // Open pipe
        secretPipe = std::make_shared<SecurePipe>(
            crow::connections::systemBus->get_io_context(), std::move(secret));
        unixFd = secretPipe->fd();

        // Pass secret over pipe
        secretPipe->asyncWrite(
            [asyncResp](const boost::system::error_code& ec, std::size_t) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Failed to pass secret: " << ec;
                messages::internalError(asyncResp->res);
            }
        });
    }

    crow::connections::systemBus->async_method_call(
        [asyncResp, secretPipe](const boost::system::error_code& ec,
                                bool success) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;
            messages::internalError(asyncResp->res);
        }
        else if (!success)
        {
            BMCWEB_LOG_ERROR << "Service responded with error";
            messages::generalError(asyncResp->res);
        }
        },
        service, "/xyz/openbmc_project/VirtualMedia/Legacy/" + name,
        "xyz.openbmc_project.VirtualMedia.Legacy", "Mount", imageUrl, rw,
        unixFd);
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
    BMCWEB_LOG_DEBUG << "Validation started";
    // required param imageUrl must not be empty
    if (!actionParams.imageUrl)
    {
        BMCWEB_LOG_ERROR << "Request action parameter Image is empty.";

        messages::propertyValueFormatError(asyncResp->res, "<empty>", "Image");

        return;
    }

    // optional param inserted must be true
    if ((actionParams.inserted != std::nullopt) && !*actionParams.inserted)
    {
        BMCWEB_LOG_ERROR
            << "Request action optional parameter Inserted must be true.";

        messages::actionParameterNotSupported(asyncResp->res, "Inserted",
                                              "InsertMedia");

        return;
    }

    // optional param transferMethod must be stream
    if ((actionParams.transferMethod != std::nullopt) &&
        (*actionParams.transferMethod != "Stream"))
    {
        BMCWEB_LOG_ERROR << "Request action optional parameter "
                            "TransferMethod must be Stream.";

        messages::actionParameterNotSupported(asyncResp->res, "TransferMethod",
                                              "InsertMedia");

        return;
    }
    boost::urls::result<boost::urls::url_view> url =
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
    if (*uriTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR << "Request action parameter ImageUrl must "
                            "contain specified protocol type from list: "
                            "(smb, https).";

        messages::resourceAtUriInUnknownFormat(asyncResp->res, *url);

        return;
    }

    // transferProtocolType should contain value from list
    if (*paramTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR << "Request action parameter TransferProtocolType "
                            "must be provided with value from list: "
                            "(CIFS, HTTPS).";

        messages::propertyValueNotInList(asyncResp->res,
                                         *actionParams.transferProtocolType,
                                         "TransferProtocolType");
        return;
    }

    // valid transfer protocol not provided either with URI nor param
    if ((uriTransferProtocolType == std::nullopt) &&
        (paramTransferProtocolType == std::nullopt))
    {
        BMCWEB_LOG_ERROR << "Request action parameter ImageUrl must "
                            "contain specified protocol type or param "
                            "TransferProtocolType must be provided.";

        messages::resourceAtUriInUnknownFormat(asyncResp->res, *url);

        return;
    }

    // valid transfer protocol provided both with URI and param
    if ((paramTransferProtocolType != std::nullopt) &&
        (uriTransferProtocolType != std::nullopt))
    {
        // check if protocol is the same for URI and param
        if (*paramTransferProtocolType != *uriTransferProtocolType)
        {
            BMCWEB_LOG_ERROR << "Request action parameter "
                                "TransferProtocolType must  contain the "
                                "same protocol type as protocol type "
                                "provided with param imageUrl.";

            messages::actionParameterValueTypeError(
                asyncResp->res, *actionParams.transferProtocolType,
                "TransferProtocolType", "InsertMedia");

            return;
        }
    }

    // validation passed, add protocol to URI if needed
    if (uriTransferProtocolType == std::nullopt)
    {
        actionParams.imageUrl = getUriWithTransferProtocol(
            *actionParams.imageUrl, *paramTransferProtocolType);
    }

    doMountVmLegacy(asyncResp, service, resName, *actionParams.imageUrl,
                    !(*actionParams.writeProtected),
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
                BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;

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
                BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;

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
    if (!json_util::readJsonAction(
            req, asyncResp->res, "Image", actionParams.imageUrl,
            "WriteProtected", actionParams.writeProtected, "UserName",
            actionParams.userName, "Password", actionParams.password,
            "Inserted", actionParams.inserted, "TransferMethod",
            actionParams.transferMethod, "TransferProtocolType",
            actionParams.transferProtocolType))
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
            BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: " << ec;
            messages::resourceNotFound(asyncResp->res, action, resName);

            return;
        }

        std::string service = getObjectType.begin()->first;
        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

        crow::connections::systemBus->async_method_call(
            [service, resName, action, actionParams,
             asyncResp](const boost::system::error_code& ec2,
                        dbus::utility::ManagedObjectType& subtree) mutable {
            if (ec2)
            {
                // Not possible in proxy mode
                BMCWEB_LOG_DEBUG << "InsertMedia not "
                                    "allowed in proxy mode";
                messages::resourceNotFound(asyncResp->res, action, resName);

                return;
            }
            for (const auto& object : subtree)
            {
                VmMode mode = parseObjectPathAndGetMode(object.first, resName);
                if (mode == VmMode::Proxy)
                {
                    validateParams(asyncResp, service, resName, actionParams);

                    return;
                }
            }
            BMCWEB_LOG_DEBUG << "Parent item not found";
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", resName);
            },
            service, "/xyz/openbmc_project/VirtualMedia",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
            BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: " << ec2;
            messages::internalError(asyncResp->res);

            return;
        }
        std::string service = getObjectType.begin()->first;
        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

        crow::connections::systemBus->async_method_call(
            [resName, service, action,
             asyncResp](const boost::system::error_code& ec,
                        const dbus::utility::ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "ObjectMapper : No Service found";
                messages::resourceNotFound(asyncResp->res, action, resName);
                return;
            }

            for (const auto& object : subtree)
            {

                VmMode mode = parseObjectPathAndGetMode(object.first, resName);
                if (mode != VmMode::Invalid)
                {
                    doEjectAction(asyncResp, service, resName,
                                  mode == VmMode::Legacy);
                }
            }
            BMCWEB_LOG_DEBUG << "Parent item not found";
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", resName);
            },
            service, "/xyz/openbmc_project/VirtualMedia",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
    asyncResp->res.jsonValue["@odata.id"] = crow::utility::urlFromPieces(
        "redfish", "v1", "Managers", name, "VirtualMedia");

    dbus::utility::getDbusObject(
        "/xyz/openbmc_project/VirtualMedia", {},
        [asyncResp, name](const boost::system::error_code& ec,
                          const dbus::utility::MapperGetObject& getObjectType) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: " << ec;
            messages::internalError(asyncResp->res);

            return;
        }
        std::string service = getObjectType.begin()->first;
        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

        getVmResourceList(asyncResp, service, name);
        });
}

inline void
    handleVirtualMediaGet(crow::App& app, const crow::Request& req,
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
            BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: " << ec;
            messages::internalError(asyncResp->res);

            return;
        }
        std::string service = getObjectType.begin()->first;
        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

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
