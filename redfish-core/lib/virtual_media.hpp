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

#include <boost/container/flat_map.hpp>
#include <boost/process/async_pipe.hpp>
#include <boost/type_traits/has_dereference.hpp>
#include <node.hpp>
#include <utils/json_utils.hpp>
// for GetObjectType and ManagedObjectType
#include <account_service.hpp>

#include <chrono>

namespace redfish

{

/**
 * @brief Read all known properties from VM object interfaces
 */
static void vmParseInterfaceObject(const DbusInterfaceType& interface,
                                   std::shared_ptr<AsyncResp> aResp)
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
            const auto imageUrlProperty =
                mountPointIface->second.find("ImageURL");
            if (imageUrlProperty != processIface->second.cend())
            {
                const std::string* imageUrlValue =
                    std::get_if<std::string>(&imageUrlProperty->second);
                if (imageUrlValue && !imageUrlValue->empty())
                {
                    aResp->res.jsonValue["ImageName"] = *imageUrlValue;
                    aResp->res.jsonValue["Inserted"] = *activeValue;
                    if (*activeValue == true)
                    {
                        aResp->res.jsonValue["ConnectedVia"] = "URI";
                    }
                }
            }
        }
    }
}

/**
 * @brief parses Timeout property and converts to microseconds
 */
static std::optional<uint64_t>
    vmParseTimeoutProperty(const std::variant<int>& timeoutProperty)
{
    const int* timeoutValue = std::get_if<int>(&timeoutProperty);
    if (timeoutValue)
    {
        constexpr int timeoutMarginSeconds = 10;
        return std::chrono::duration_cast<std::chrono::microseconds>(
                   std::chrono::seconds(*timeoutValue + timeoutMarginSeconds))
            .count();
    }
    else
    {
        return std::nullopt;
    }
}

/**
 * @brief Fill template for Virtual Media Item.
 */
static nlohmann::json vmItemTemplate(const std::string& name,
                                     const std::string& resName)
{
    nlohmann::json item;
    item["@odata.id"] =
        "/redfish/v1/Managers/" + name + "/VirtualMedia/" + resName;
    item["@odata.type"] = "#VirtualMedia.v1_3_0.VirtualMedia";
    item["Name"] = "Virtual Removable Media";
    item["Id"] = resName;
    item["Image"] = nullptr;
    item["Inserted"] = nullptr;
    item["ImageName"] = nullptr;
    item["WriteProtected"] = true;
    item["ConnectedVia"] = "NotConnected";
    item["MediaTypes"] = {"CD", "USBStick"};
    item["TransferMethod"] = "Stream";
    item["TransferProtocolType"] = nullptr;
    item["Oem"]["OpenBmc"]["WebSocketEndpoint"] = nullptr;
    item["Oem"]["OpenBMC"]["@odata.type"] =
        "#OemVirtualMedia.v1_0_0.VirtualMedia";

    return item;
}

/**
 *  @brief Fills collection data
 */
static void getVmResourceList(std::shared_ptr<AsyncResp> aResp,
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
                const std::string& path =
                    static_cast<const std::string&>(object.first);
                std::size_t lastIndex = path.rfind("/");
                if (lastIndex == std::string::npos)
                {
                    continue;
                }

                lastIndex += 1;

                item["@odata.id"] = "/redfish/v1/Managers/" + name +
                                    "/VirtualMedia/" + path.substr(lastIndex);

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
static void getVmData(std::shared_ptr<AsyncResp> aResp,
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
                const std::string& path =
                    static_cast<const std::string&>(item.first);

                std::size_t lastItem = path.rfind("/");
                if (lastItem == std::string::npos)
                {
                    continue;
                }

                if (path.substr(lastItem + 1) != resName)
                {
                    continue;
                }

                aResp->res.jsonValue = vmItemTemplate(name, resName);

                // Check if dbus path is Legacy type
                if (path.find("VirtualMedia/Legacy") != std::string::npos)
                {
                    aResp->res.jsonValue["Actions"]["#VirtualMedia.InsertMedia"]
                                        ["target"] =
                        "/redfish/v1/Managers/" + name + "/VirtualMedia/" +
                        resName + "/Actions/VirtualMedia.InsertMedia";
                }

                vmParseInterfaceObject(item.second, aResp);

                aResp->res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"]
                                    ["target"] =
                    "/redfish/v1/Managers/" + name + "/VirtualMedia/" +
                    resName + "/Actions/VirtualMedia.EjectMedia";

                return;
            }

            messages::resourceNotFound(
                aResp->res, "#VirtualMedia.v1_3_0.VirtualMedia", resName);
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
   @brief InsertMedia action class
 */
class VirtualMediaActionInsertMedia : public Node
{
  public:
    VirtualMediaActionInsertMedia(CrowApp& app) :
        Node(app,
             "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/"
             "VirtualMedia.InsertMedia",
             std::string(), std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
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
    std::optional<TransferProtocol>
        getTransferProtocolFromUri(const std::string& imageUri)
    {
        if (imageUri.find("smb://") != std::string::npos)
        {
            return TransferProtocol::smb;
        }
        else if (imageUri.find("https://") != std::string::npos)
        {
            return TransferProtocol::https;
        }
        else if (imageUri.find("://") != std::string::npos)
        {
            return TransferProtocol::invalid;
        }
        else
        {
            return {};
        }
    }

    /**
     * @brief Function convert transfer protocol from string param.
     *
     */
    std::optional<TransferProtocol> getTransferProtocolFromParam(
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
    const std::string
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
    bool validateParams(crow::Response& res, std::string& imageUrl,
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

            messages::actionParameterNotSupported(res, "Inserted",
                                                  "InsertMedia");

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

                messages::actionParameterValueTypeError(
                    res, *transferProtocolType, "TransferProtocolType",
                    "InsertMedia");

                return false;
            }
        }

        // validation passed
        // add protocol to URI if needed
        if (uriTransferProtocolType == std::nullopt)
        {
            imageUrl = getUriWithTransferProtocol(imageUrl,
                                                  *paramTransferProtocolType);
        }

        return true;
    }

    /**
     * @brief Function handles POST method request.
     *
     * Analyzes POST body message before sends Reset request data to dbus.
     */
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto aResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 2)
        {
            messages::internalError(res);
            return;
        }

        // take resource name from URL
        const std::string& resName = params[1];

        if (params[0] != "bmc")
        {
            messages::resourceNotFound(res, "VirtualMedia.Insert", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [this, aResp{std::move(aResp)}, req,
             resName](const boost::system::error_code ec,
                      const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                     << ec;
                    messages::internalError(aResp->res);

                    return;
                }
                std::string service = getObjectType.begin()->first;
                BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                crow::connections::systemBus->async_method_call(
                    [this, service, resName, req, aResp{std::move(aResp)}](
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
                                static_cast<const std::string&>(object.first);

                            std::size_t lastIndex = path.rfind("/");
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
                                    BMCWEB_LOG_DEBUG << "InsertMedia not "
                                                        "allowed in proxy mode";
                                    messages::resourceNotFound(
                                        aResp->res, "VirtualMedia.InsertMedia",
                                        resName);

                                    return;
                                }

                                lastIndex = path.rfind("Legacy");
                                if (lastIndex == std::string::npos)
                                {
                                    continue;
                                }

                                // Legacy mode
                                std::string imageUrl;
                                std::optional<std::string> userName;
                                std::optional<std::string> password;
                                std::optional<std::string> transferMethod;
                                std::optional<std::string> transferProtocolType;
                                std::optional<bool> writeProtected = true;
                                std::optional<bool> inserted;

                                // Read obligatory parameters (url of image)
                                if (!json_util::readJson(
                                        req, aResp->res, "Image", imageUrl,
                                        "WriteProtected", writeProtected,
                                        "UserName", userName, "Password",
                                        password, "Inserted", inserted,
                                        "TransferMethod", transferMethod,
                                        "TransferProtocolType",
                                        transferProtocolType))
                                {
                                    BMCWEB_LOG_DEBUG << "Image is not provided";
                                    return;
                                }

                                bool paramsValid = validateParams(
                                    aResp->res, imageUrl, inserted,
                                    transferMethod, transferProtocolType);

                                if (paramsValid == false)
                                {
                                    return;
                                }

                                // manager is irrelevant for VirtualMedia dbus
                                // calls
                                doMountVmLegacy(
                                    std::move(aResp), service, resName,
                                    imageUrl, !(*writeProtected),
                                    std::move(*userName), std::move(*password));

                                return;
                            }
                        }
                        BMCWEB_LOG_DEBUG << "Parent item not found";
                        messages::resourceNotFound(aResp->res, "VirtualMedia",
                                                   resName);
                    },
                    service, "/xyz/openbmc_project/VirtualMedia",
                    "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/VirtualMedia", std::array<const char*, 0>());
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

      private:
        Credentials() = delete;
        Credentials(const Credentials&) = delete;
        Credentials& operator=(const Credentials&) = delete;

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

        SecureBuffer pack(const FormatterFunc formatter)
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
        void async_write(WriteHandler&& handler)
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
    void doMountVmLegacy(std::shared_ptr<AsyncResp> asyncResp,
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
            auto secret = credentials.pack([](const auto& user,
                                              const auto& pass, auto& buff) {
                std::copy(user.begin(), user.end(), std::back_inserter(buff));
                buff.push_back('\0');
                std::copy(pass.begin(), pass.end(), std::back_inserter(buff));
                buff.push_back('\0');
            });

            // Open pipe
            secretPipe = std::make_shared<SecurePipe>(
                crow::connections::systemBus->get_io_context(),
                std::move(secret));
            unixFd = secretPipe->fd();

            // Pass secret over pipe
            secretPipe->async_write(
                [asyncResp](const boost::system::error_code& ec,
                            std::size_t size) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Failed to pass secret: " << ec;
                        messages::internalError(asyncResp->res);
                    }
                });
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, service, name, imageUrl, rw, unixFd,
             secretPipe](const boost::system::error_code ec,
                         const std::variant<int> timeoutProperty) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto timeout = vmParseTimeoutProperty(timeoutProperty);
                if (timeout == std::nullopt)
                {
                    BMCWEB_LOG_ERROR << "Timeout property is empty.";
                    messages::internalError(asyncResp->res);
                    return;
                }

                crow::connections::systemBus->async_method_call_timed(
                    [asyncResp, secretPipe](const boost::system::error_code ec,
                                            bool success) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Bad D-Bus request error: "
                                             << ec;
                            if (ec ==
                                boost::system::errc::device_or_resource_busy)
                            {
                                messages::resourceInUse(asyncResp->res);
                            }
                            else
                            {
                                messages::internalError(asyncResp->res);
                            }
                        }
                        else if (!success)
                        {
                            BMCWEB_LOG_ERROR << "Service responded with error";
                            messages::generalError(asyncResp->res);
                        }
                    },
                    service, "/xyz/openbmc_project/VirtualMedia/Legacy/" + name,
                    "xyz.openbmc_project.VirtualMedia.Legacy", "Mount",
                    *timeout, imageUrl, rw, unixFd);
            },
            service, "/xyz/openbmc_project/VirtualMedia/Legacy/" + name,
            "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.VirtualMedia.MountPoint", "Timeout");
    }
};

/**
   @brief EjectMedia action class
 */
class VirtualMediaActionEjectMedia : public Node
{
  public:
    VirtualMediaActionEjectMedia(CrowApp& app) :
        Node(app,
             "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/"
             "VirtualMedia.EjectMedia",
             std::string(), std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * @brief Function handles POST method request.
     *
     * Analyzes POST body message before sends Reset request data to dbus.
     */
    void doPost(crow::Response& res, const crow::Request& req,
                const std::vector<std::string>& params) override
    {
        auto aResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 2)
        {
            messages::internalError(res);
            return;
        }

        // take resource name from URL
        const std::string& resName = params[1];

        if (params[0] != "bmc")
        {
            messages::resourceNotFound(res, "VirtualMedia.Eject", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [this, aResp{std::move(aResp)}, req,
             resName](const boost::system::error_code ec,
                      const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
                                     << ec;
                    messages::internalError(aResp->res);

                    return;
                }
                std::string service = getObjectType.begin()->first;
                BMCWEB_LOG_DEBUG << "GetObjectType: " << service;

                crow::connections::systemBus->async_method_call(
                    [this, resName, service, req, aResp{std::move(aResp)}](
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
                                static_cast<const std::string&>(object.first);

                            std::size_t lastIndex = path.rfind("/");
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
                                    doVmAction(std::move(aResp), service,
                                               resName, false);
                                }

                                lastIndex = path.rfind("Legacy");
                                if (lastIndex != std::string::npos)
                                {
                                    // Legacy mode
                                    doVmAction(std::move(aResp), service,
                                               resName, true);
                                }

                                return;
                            }
                        }
                        BMCWEB_LOG_DEBUG << "Parent item not found";
                        messages::resourceNotFound(aResp->res, "VirtualMedia",
                                                   resName);
                    },
                    service, "/xyz/openbmc_project/VirtualMedia",
                    "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetObject",
            "/xyz/openbmc_project/VirtualMedia", std::array<const char*, 0>());
    }

    /**
     * @brief Function transceives data with dbus directly.
     *
     * All BMC state properties will be retrieved before sending reset request.
     */
    void doVmAction(std::shared_ptr<AsyncResp> asyncResp,
                    const std::string& service, const std::string& name,
                    bool legacy)
    {
        std::string objectPath = "/xyz/openbmc_project/VirtualMedia/";
        std::string ifaceName = "xyz.openbmc_project.VirtualMedia";
        if (legacy)
        {
            objectPath += "Legacy/";
            ifaceName += ".Legacy";
        }
        else
        {
            objectPath += "Proxy/";
            ifaceName += ".Proxy";
        }
        objectPath += name;

        crow::connections::systemBus->async_method_call(
            [asyncResp, service, name, objectPath,
             ifaceName](const boost::system::error_code ec,
                        const std::variant<int> timeoutProperty) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto timeout = vmParseTimeoutProperty(timeoutProperty);
                if (timeout == std::nullopt)
                {
                    BMCWEB_LOG_ERROR << "Timeout property is empty.";
                    messages::internalError(asyncResp->res);
                    return;
                }
                crow::connections::systemBus->async_method_call_timed(
                    [asyncResp](const boost::system::error_code ec) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR << "Bad D-Bus request error: "
                                             << ec;
                            if (ec ==
                                boost::system::errc::device_or_resource_busy)
                            {
                                messages::resourceInUse(asyncResp->res);
                            }
                            else
                            {
                                messages::internalError(asyncResp->res);
                            }
                            return;
                        }
                    },
                    service, objectPath, ifaceName, "Unmount", *timeout);
            },
            service, objectPath, "org.freedesktop.DBus.Properties", "Get",
            "xyz.openbmc_project.VirtualMedia.MountPoint", "Timeout");
    }
};

class VirtualMediaCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    VirtualMediaCollection(CrowApp& app) :
        Node(app, "/redfish/v1/Managers/<str>/VirtualMedia/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            return;
        }

        const std::string& name = params[0];

        if (name != "bmc")
        {
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", name);

            return;
        }

        res.jsonValue["@odata.type"] =
            "#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["Name"] = "Virtual Media Services";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/" + name + "/VirtualMedia/";

        crow::connections::systemBus->async_method_call(
            [asyncResp, name](const boost::system::error_code ec,
                              const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char*, 0>());
    }
};

class VirtualMedia : public Node
{
  public:
    /*
     * Default Constructor
     */
    VirtualMedia(CrowApp& app) :
        Node(app, "/redfish/v1/Managers/<str>/VirtualMedia/<str>/",
             std::string(), std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureManager"}}},
            {boost::beast::http::verb::put, {{"ConfigureManager"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureManager"}}},
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 2)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& name = params[0];
        const std::string& resName = params[1];

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (name != "bmc")
        {
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, name, resName](const boost::system::error_code ec,
                                       const GetObjectType& getObjectType) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "ObjectMapper::GetObject call failed: "
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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char*, 0>());
    }
};

} // namespace redfish
