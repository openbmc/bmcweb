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

#include <app.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/type_traits/has_dereference.hpp>
#include <utils/json_utils.hpp>
// for GetObjectType and ManagedObjectType

#include <account_service.hpp>
#include <boost/url/url_view.hpp>
#include <registries/privilege_registry.hpp>

namespace redfish
{
/**
 * @brief Function extracts transfer protocol name from URI.
 */
inline std::string getTransferProtocolTypeFromUri(const std::string& imageUri)
{
    boost::urls::result<boost::urls::url_view> url =
        boost::urls::parse_uri(boost::string_view(imageUri));
    if (!url)
    {
        return "None";
    }
    boost::string_view scheme = url->scheme();
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
    vmParseInterfaceObject(const DbusInterfaceType& interface,
                           const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    const auto mountPointIface =
        interface.find("xyz.openbmc_project.VirtualMedia.MountPoint");
    if (mountPointIface == interface.cend())
    {
        BMCWEB_LOG_DEBUG << "Interface MountPoint not found";
        return;
    }

    const auto processIface =
        interface.find("xyz.openbmc_project.VirtualMedia.Process");
    if (processIface == interface.cend())
    {
        BMCWEB_LOG_DEBUG << "Interface Process not found";
        return;
    }

    const auto endpointIdProperty = mountPointIface->second.find("EndpointId");
    if (endpointIdProperty == mountPointIface->second.cend())
    {
        BMCWEB_LOG_DEBUG << "Property EndpointId not found";
        return;
    }

    const auto activeProperty = processIface->second.find("Active");
    if (activeProperty == processIface->second.cend())
    {
        BMCWEB_LOG_DEBUG << "Property Active not found";
        return;
    }

    const bool* activeValue = std::get_if<bool>(&activeProperty->second);
    if (!activeValue)
    {
        BMCWEB_LOG_DEBUG << "Value Active not found";
        return;
    }

    const std::string* endpointIdValue =
        std::get_if<std::string>(&endpointIdProperty->second);
    if (endpointIdValue)
    {
        if (!endpointIdValue->empty())
        {
            // Proxy mode
            aResp->res.jsonValue["Oem"]["OpenBMC"]["WebSocketEndpoint"] =
                *endpointIdValue;
            aResp->res.jsonValue["TransferProtocolType"] = "OEM";
            aResp->res.jsonValue["Inserted"] = *activeValue;
            if (*activeValue == true)
            {
                aResp->res.jsonValue["ConnectedVia"] = "Applet";
            }
        }
        else
        {
            // Legacy mode
            for (const auto& property : mountPointIface->second)
            {
                if (property.first == "ImageURL")
                {
                    const std::string* imageUrlValue =
                        std::get_if<std::string>(&property.second);
                    if (imageUrlValue && !imageUrlValue->empty())
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
                        aResp->res.jsonValue["Inserted"] = *activeValue;
                        aResp->res.jsonValue["TransferProtocolType"] =
                            getTransferProtocolTypeFromUri(*imageUrlValue);

                        if (*activeValue == true)
                        {
                            aResp->res.jsonValue["ConnectedVia"] = "URI";
                        }
                    }
                }
                else if (property.first == "WriteProtected")
                {
                    const bool* writeProtectedValue =
                        std::get_if<bool>(&property.second);
                    if (writeProtectedValue)
                    {
                        aResp->res.jsonValue["WriteProtected"] =
                            *writeProtectedValue;
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

    std::string id = "/redfish/v1/Managers/";
    id += name;
    id += "/VirtualMedia/";
    id += resName;
    item["@odata.id"] = std::move(id);

    item["@odata.type"] = "#VirtualMedia.v1_3_0.VirtualMedia";
    item["Name"] = "Virtual Removable Media";
    item["Id"] = resName;
    item["WriteProtected"] = true;
    item["MediaTypes"] = {"CD", "USBStick"};
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
        [name, aResp{std::move(aResp)}](const boost::system::error_code ec,
                                        ManagedObjectType& subtree) {
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

                std::string id = "/redfish/v1/Managers/";
                id += name;
                id += "/VirtualMedia/";
                id += path;

                item["@odata.id"] = std::move(id);
                members.emplace_back(std::move(item));
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 *  @brief Fills data for specific resource
 */
inline void getVmData(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                      const std::string& service, const std::string& name,
                      const std::string& resName)
{
    BMCWEB_LOG_DEBUG << "Get Virtual Media resource data.";

    crow::connections::systemBus->async_method_call(
        [resName, name, aResp](const boost::system::error_code ec,
                               ManagedObjectType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";

                return;
            }

            for (auto& item : subtree)
            {
                std::string thispath = item.first.filename();
                if (thispath.empty())
                {
                    continue;
                }

                if (thispath != resName)
                {
                    continue;
                }

                // "Legacy"/"Proxy"
                auto mode = item.first.parent_path();
                // "VirtualMedia"
                auto type = mode.parent_path();
                if (mode.filename().empty() || type.filename().empty())
                {
                    continue;
                }

                if (type.filename() != "VirtualMedia")
                {
                    continue;
                }

                aResp->res.jsonValue = vmItemTemplate(name, resName);
                std::string actionsId = "/redfish/v1/Managers/";
                actionsId += name;
                actionsId += "/VirtualMedia/";
                actionsId += resName;
                actionsId += "/Actions";

                // Check if dbus path is Legacy type
                if (mode.filename() == "Legacy")
                {
                    aResp->res.jsonValue["Actions"]["#VirtualMedia.InsertMedia"]
                                        ["target"] =
                        actionsId + "/VirtualMedia.InsertMedia";
                }

                vmParseInterfaceObject(item.second, aResp);

                aResp->res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"]
                                    ["target"] =
                    actionsId + "/VirtualMedia.EjectMedia";

                return;
            }

            messages::resourceNotFound(
                aResp->res, "#VirtualMedia.v1_3_0.VirtualMedia", resName);
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
    getTransferProtocolFromUri(const std::string& imageUri)
{
    boost::urls::result<boost::urls::url_view> url =
        boost::urls::parse_uri(boost::string_view(imageUri));
    if (!url)
    {
        return {};
    }

    boost::string_view scheme = url->scheme();
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

/**
 * @brief Function validate parameters of insert media request.
 *
 */
inline bool
    validateParams(crow::Response& res, std::string& imageUrl,
                   const std::optional<bool>& inserted,
                   const std::optional<std::string>& transferMethod,
                   const std::optional<std::string>& transferProtocolType)
{
    BMCWEB_LOG_DEBUG << "Validation started";
    // required param imageUrl must not be empty
    if (imageUrl.empty())
    {
        BMCWEB_LOG_ERROR << "Request action parameter Image is empty.";

        messages::propertyValueFormatError(res, "<empty>", "Image");

        return false;
    }

    // optional param inserted must be true
    if ((inserted != std::nullopt) && (*inserted != true))
    {
        BMCWEB_LOG_ERROR
            << "Request action optional parameter Inserted must be true.";

        messages::actionParameterNotSupported(res, "Inserted", "InsertMedia");

        return false;
    }

    // optional param transferMethod must be stream
    if ((transferMethod != std::nullopt) && (*transferMethod != "Stream"))
    {
        BMCWEB_LOG_ERROR << "Request action optional parameter "
                            "TransferMethod must be Stream.";

        messages::actionParameterNotSupported(res, "TransferMethod",
                                              "InsertMedia");

        return false;
    }

    std::optional<TransferProtocol> uriTransferProtocolType =
        getTransferProtocolFromUri(imageUrl);

    std::optional<TransferProtocol> paramTransferProtocolType =
        getTransferProtocolFromParam(transferProtocolType);

    // ImageUrl does not contain valid protocol type
    if (*uriTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR << "Request action parameter ImageUrl must "
                            "contain specified protocol type from list: "
                            "(smb, https).";

        messages::resourceAtUriInUnknownFormat(res, imageUrl);

        return false;
    }

    // transferProtocolType should contain value from list
    if (*paramTransferProtocolType == TransferProtocol::invalid)
    {
        BMCWEB_LOG_ERROR << "Request action parameter TransferProtocolType "
                            "must be provided with value from list: "
                            "(CIFS, HTTPS).";

        messages::propertyValueNotInList(res, *transferProtocolType,
                                         "TransferProtocolType");
        return false;
    }

    // valid transfer protocol not provided either with URI nor param
    if ((uriTransferProtocolType == std::nullopt) &&
        (paramTransferProtocolType == std::nullopt))
    {
        BMCWEB_LOG_ERROR << "Request action parameter ImageUrl must "
                            "contain specified protocol type or param "
                            "TransferProtocolType must be provided.";

        messages::resourceAtUriInUnknownFormat(res, imageUrl);

        return false;
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

            messages::actionParameterValueTypeError(res, *transferProtocolType,
                                                    "TransferProtocolType",
                                                    "InsertMedia");

            return false;
        }
    }

    // validation passed
    // add protocol to URI if needed
    if (uriTransferProtocolType == std::nullopt)
    {
        imageUrl =
            getUriWithTransferProtocol(imageUrl, *paramTransferProtocolType);
    }

    return true;
}

template <typename T>
static void secureCleanup(T& value)
{
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

    SecureBuffer pack(FormatterFunc formatter)
    {
        SecureBuffer packed{new Buffer{}};
        if (formatter)
        {
            formatter(credentials.user(), credentials.password(), *packed);
        }

        return packed;
    }

  private:
    Credentials credentials;
};

// Wrapper for boost::async_pipe ensuring proper pipe cleanup
template <typename Buffer>
class Pipe
{
  public:
    using unix_fd = sdbusplus::message::unix_fd;

    Pipe(boost::asio::io_context& io, Buffer&& buffer) :
        impl(io), buffer{std::move(buffer)}
    {}

    ~Pipe()
    {
        // Named pipe needs to be explicitly removed
        impl.close();
    }

    unix_fd fd()
    {
        return unix_fd{impl.native_source()};
    }

    template <typename WriteHandler>
    void asyncWrite(WriteHandler&& handler)
    {
        impl.async_write_some(data(), std::forward<WriteHandler>(handler));
    }

  private:
    // Specialization for pointer types
    template <typename B = Buffer>
    typename std::enable_if<boost::has_dereference<B>::value,
                            boost::asio::const_buffer>::type
        data()
    {
        return boost::asio::buffer(*buffer);
    }

    template <typename B = Buffer>
    typename std::enable_if<!boost::has_dereference<B>::value,
                            boost::asio::const_buffer>::type
        data()
    {
        return boost::asio::buffer(buffer);
    }

    const std::string name;
    boost::process::async_pipe impl;
    Buffer buffer;
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
    using SecurePipe = Pipe<CredentialsProvider::SecureBuffer>;
    constexpr const size_t secretLimit = 1024;

    std::shared_ptr<SecurePipe> secretPipe;
    std::variant<int, SecurePipe::unix_fd> unixFd = -1;

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
        [asyncResp, secretPipe](const boost::system::error_code ec,
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
 * @brief Function transceives data with dbus directly.
 *
 * All BMC state properties will be retrieved before sending reset request.
 */
inline void doVmAction(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& service, const std::string& name,
                       bool legacy)
{

    // Legacy mount requires parameter with image
    if (legacy)
    {
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
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
            [asyncResp](const boost::system::error_code ec) {
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

struct InsertMediaActionParams
{
    std::string imageUrl;
    std::optional<std::string> userName;
    std::optional<std::string> password;
    std::optional<std::string> transferMethod;
    std::optional<std::string> transferProtocolType;
    std::optional<bool> writeProtected = true;
    std::optional<bool> inserted;
};

inline void requestNBDVirtualMediaRoutes(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.InsertMedia")
        .privileges(redfish::privileges::postVirtualMedia)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name, const std::string& resName) {
                if (name != "bmc")
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "VirtualMedia.Insert", resName);

                    return;
                }
                InsertMediaActionParams actionParams;

                // Read obligatory parameters (url of
                // image)
                if (!json_util::readJson(
                        req, asyncResp->res, "Image", actionParams.imageUrl,
                        "WriteProtected", actionParams.writeProtected,
                        "UserName", actionParams.userName, "Password",
                        actionParams.password, "Inserted",
                        actionParams.inserted, "TransferMethod",
                        actionParams.transferMethod, "TransferProtocolType",
                        actionParams.transferProtocolType))
                {
                    BMCWEB_LOG_DEBUG << "Image is not provided";
                    return;
                }

                bool paramsValid = validateParams(
                    asyncResp->res, actionParams.imageUrl,
                    actionParams.inserted, actionParams.transferMethod,
                    actionParams.transferProtocolType);

                if (paramsValid == false)
                {
                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, actionParams,
                     resName](const boost::system::error_code ec,
                              const GetObjectType& getObjectType) mutable {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "ObjectMapper::GetObject call failed: "
                                << ec;
                            messages::internalError(asyncResp->res);

                            return;
                        }
                        std::string service = getObjectType.begin()->first;
                        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                        crow::connections::systemBus->async_method_call(
                            [service, resName, actionParams,
                             asyncResp](const boost::system::error_code ec,
                                        ManagedObjectType& subtree) mutable {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG << "DBUS response error";

                                    return;
                                }

                                for (const auto& object : subtree)
                                {
                                    const std::string& path =
                                        static_cast<const std::string&>(
                                            object.first);

                                    std::size_t lastIndex = path.rfind('/');
                                    if (lastIndex == std::string::npos)
                                    {
                                        continue;
                                    }

                                    lastIndex += 1;

                                    if (path.substr(lastIndex) == resName)
                                    {
                                        lastIndex = path.rfind("Proxy");
                                        if (lastIndex != std::string::npos)
                                        {
                                            // Not possible in proxy mode
                                            BMCWEB_LOG_DEBUG
                                                << "InsertMedia not "
                                                   "allowed in proxy mode";
                                            messages::resourceNotFound(
                                                asyncResp->res,
                                                "VirtualMedia.InsertMedia",
                                                resName);

                                            return;
                                        }

                                        lastIndex = path.rfind("Legacy");
                                        if (lastIndex == std::string::npos)
                                        {
                                            continue;
                                        }

                                        // manager is irrelevant for
                                        // VirtualMedia dbus calls
                                        doMountVmLegacy(
                                            asyncResp, service, resName,
                                            actionParams.imageUrl,
                                            !(*actionParams.writeProtected),
                                            std::move(*actionParams.userName),
                                            std::move(*actionParams.password));

                                        return;
                                    }
                                }
                                BMCWEB_LOG_DEBUG << "Parent item not found";
                                messages::resourceNotFound(
                                    asyncResp->res, "VirtualMedia", resName);
                            },
                            service, "/xyz/openbmc_project/VirtualMedia",
                            "org.freedesktop.DBus.ObjectManager",
                            "GetManagedObjects");
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/VirtualMedia",
                    std::array<const char*, 0>());
            });

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/VirtualMedia.EjectMedia")
        .privileges(redfish::privileges::postVirtualMedia)
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name, const std::string& resName) {
                if (name != "bmc")
                {
                    messages::resourceNotFound(asyncResp->res,
                                               "VirtualMedia.Eject", resName);

                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, resName](const boost::system::error_code ec,
                                         const GetObjectType& getObjectType) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "ObjectMapper::GetObject call failed: "
                                << ec;
                            messages::internalError(asyncResp->res);

                            return;
                        }
                        std::string service = getObjectType.begin()->first;
                        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                        crow::connections::systemBus->async_method_call(
                            [resName, service, asyncResp{asyncResp}](
                                const boost::system::error_code ec,
                                ManagedObjectType& subtree) {
                                if (ec)
                                {
                                    BMCWEB_LOG_DEBUG << "DBUS response error";

                                    return;
                                }

                                for (const auto& object : subtree)
                                {
                                    const std::string& path =
                                        static_cast<const std::string&>(
                                            object.first);

                                    std::size_t lastIndex = path.rfind('/');
                                    if (lastIndex == std::string::npos)
                                    {
                                        continue;
                                    }

                                    lastIndex += 1;

                                    if (path.substr(lastIndex) == resName)
                                    {
                                        lastIndex = path.rfind("Proxy");
                                        if (lastIndex != std::string::npos)
                                        {
                                            // Proxy mode
                                            doVmAction(asyncResp, service,
                                                       resName, false);
                                        }

                                        lastIndex = path.rfind("Legacy");
                                        if (lastIndex != std::string::npos)
                                        {
                                            // Legacy mode
                                            doVmAction(asyncResp, service,
                                                       resName, true);
                                        }

                                        return;
                                    }
                                }
                                BMCWEB_LOG_DEBUG << "Parent item not found";
                                messages::resourceNotFound(
                                    asyncResp->res, "VirtualMedia", resName);
                            },
                            service, "/xyz/openbmc_project/VirtualMedia",
                            "org.freedesktop.DBus.ObjectManager",
                            "GetManagedObjects");
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/VirtualMedia",
                    std::array<const char*, 0>());
            });
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/VirtualMedia/")
        .privileges(redfish::privileges::getVirtualMediaCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /* req */,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name) {
                if (name != "bmc")
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMedia",
                                               name);

                    return;
                }

                asyncResp->res.jsonValue["@odata.type"] =
                    "#VirtualMediaCollection.VirtualMediaCollection";
                asyncResp->res.jsonValue["Name"] = "Virtual Media Services";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/" + name + "/VirtualMedia";

                crow::connections::systemBus->async_method_call(
                    [asyncResp, name](const boost::system::error_code ec,
                                      const GetObjectType& getObjectType) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "ObjectMapper::GetObject call failed: "
                                << ec;
                            messages::internalError(asyncResp->res);

                            return;
                        }
                        std::string service = getObjectType.begin()->first;
                        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                        getVmResourceList(asyncResp, service, name);
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/VirtualMedia",
                    std::array<const char*, 0>());
            });

    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>/VirtualMedia/<str>/")
        .privileges(redfish::privileges::getVirtualMedia)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /* req */,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name, const std::string& resName) {
                if (name != "bmc")
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMedia",
                                               resName);

                    return;
                }

                crow::connections::systemBus->async_method_call(
                    [asyncResp, name,
                     resName](const boost::system::error_code ec,
                              const GetObjectType& getObjectType) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "ObjectMapper::GetObject call failed: "
                                << ec;
                            messages::internalError(asyncResp->res);

                            return;
                        }
                        std::string service = getObjectType.begin()->first;
                        BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                        getVmData(asyncResp, service, name, resName);
                    },
                    "xyz.openbmc_project.ObjectMapper",
                    "/xyz/openbmc_project/object_mapper",
                    "xyz.openbmc_project.ObjectMapper", "GetObject",
                    "/xyz/openbmc_project/VirtualMedia",
                    std::array<const char*, 0>());
            });
}

} // namespace redfish
