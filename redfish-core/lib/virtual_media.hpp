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
#include <node.hpp>
#include <utils/json_utils.hpp>
// for GetObjectType and ManagedObjectType
#include <account_service.hpp>

namespace redfish

{

/**
 * @brief Read all known properties from VM object interfaces
 */
static void vmParseInterfaceObject(const DbusInterfaceType &interface,
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

    const bool *activeValue = std::get_if<bool>(&activeProperty->second);
    if (!activeValue)
    {
        BMCWEB_LOG_DEBUG << "Value Active not found";
        return;
    }

    const std::string *endpointIdValue =
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
                const std::string *imageUrlValue =
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
 * @brief Fill template for Virtual Media Item.
 */
static nlohmann::json vmItemTemplate(const std::string &name,
                                     const std::string &resName)
{
    nlohmann::json item;
    item["@odata.id"] =
        "/redfish/v1/Managers/" + name + "/VirtualMedia/" + resName;
    item["@odata.type"] = "#VirtualMedia.v1_3_0.VirtualMedia";
    item["@odata.context"] = "/redfish/v1/$metadata#VirtualMedia.VirtualMedia";
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
                              const std::string &service,
                              const std::string &name)
{
    BMCWEB_LOG_DEBUG << "Get available Virtual Media resources.";
    crow::connections::systemBus->async_method_call(
        [name, aResp{std::move(aResp)}](const boost::system::error_code ec,
                                        ManagedObjectType &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                return;
            }
            nlohmann::json &members = aResp->res.jsonValue["Members"];
            members = nlohmann::json::array();

            for (const auto &object : subtree)
            {
                nlohmann::json item;
                const std::string &path =
                    static_cast<const std::string &>(object.first);
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
                      const std::string &service, const std::string &name,
                      const std::string &resName)
{
    BMCWEB_LOG_DEBUG << "Get Virtual Media resource data.";

    crow::connections::systemBus->async_method_call(
        [resName, name, aResp](const boost::system::error_code ec,
                               ManagedObjectType &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";

                return;
            }

            for (auto &item : subtree)
            {
                const std::string &path =
                    static_cast<const std::string &>(item.first);

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
    VirtualMediaActionInsertMedia(CrowApp &app) :
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
     * @brief Function handles POST method request.
     *
     * Analyzes POST body message before sends Reset request data to dbus.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto aResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 2)
        {
            messages::internalError(res);
            return;
        }

        // take resource name from URL
        const std::string &resName = params[1];

        if (params[0] != "bmc")
        {
            messages::resourceNotFound(res, "VirtualMedia.Insert", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [this, aResp{std::move(aResp)}, req,
             resName](const boost::system::error_code ec,
                      const GetObjectType &getObjectType) {
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
                        ManagedObjectType &subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";

                            return;
                        }

                        for (const auto &object : subtree)
                        {
                            const std::string &path =
                                static_cast<const std::string &>(object.first);

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
                                if (lastIndex != std::string::npos)
                                {
                                    // Legacy mode
                                    std::string imageUrl;

                                    // Read obligatory paramters (url of image)
                                    if (!json_util::readJson(req, aResp->res,
                                                             "Image", imageUrl))
                                    {
                                        BMCWEB_LOG_DEBUG
                                            << "Image is not provided";
                                        return;
                                    }

                                    // must not be empty
                                    if (imageUrl.size() == 0)
                                    {
                                        BMCWEB_LOG_ERROR
                                            << "Request action parameter "
                                               "Image is empty.";
                                        messages::propertyValueFormatError(
                                            aResp->res, "<empty>", "Image");

                                        return;
                                    }

                                    // manager is irrelevant for VirtualMedia
                                    // dbus calls
                                    doVmAction(std::move(aResp), service,
                                               resName, true, imageUrl);

                                    return;
                                }
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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char *, 0>());
    }

    /**
     * @brief Function transceives data with dbus directly.
     *
     * All BMC state properties will be retrieved before sending reset request.
     */
    void doVmAction(std::shared_ptr<AsyncResp> asyncResp,
                    const std::string &service, const std::string &name,
                    bool legacy, const std::string &imageUrl)
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
                "xyz.openbmc_project.VirtualMedia.Legacy", "Mount", imageUrl);
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
                "xyz.openbmc_project.VirtualMedia.Proxy", "Mount");
        }
    }
};

/**
   @brief EjectMedia action class
 */
class VirtualMediaActionEjectMedia : public Node
{
  public:
    VirtualMediaActionEjectMedia(CrowApp &app) :
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
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto aResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 2)
        {
            messages::internalError(res);
            return;
        }

        // take resource name from URL
        const std::string &resName = params[1];

        if (params[0] != "bmc")
        {
            messages::resourceNotFound(res, "VirtualMedia.Eject", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [this, aResp{std::move(aResp)}, req,
             resName](const boost::system::error_code ec,
                      const GetObjectType &getObjectType) {
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
                        ManagedObjectType &subtree) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG << "DBUS response error";

                            return;
                        }

                        for (const auto &object : subtree)
                        {
                            const std::string &path =
                                static_cast<const std::string &>(object.first);

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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char *, 0>());
    }

    /**
     * @brief Function transceives data with dbus directly.
     *
     * All BMC state properties will be retrieved before sending reset request.
     */
    void doVmAction(std::shared_ptr<AsyncResp> asyncResp,
                    const std::string &service, const std::string &name,
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
};

class VirtualMediaCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    VirtualMediaCollection(CrowApp &app) :
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            return;
        }

        const std::string &name = params[0];

        if (name != "bmc")
        {
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", name);

            return;
        }

        res.jsonValue["@odata.type"] =
            "#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["Name"] = "Virtual Media Services";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/" + name + "/VirtualMedia/";

        crow::connections::systemBus->async_method_call(
            [asyncResp, name](const boost::system::error_code ec,
                              const GetObjectType &getObjectType) {
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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char *, 0>());
    }
};

class VirtualMedia : public Node
{
  public:
    /*
     * Default Constructor
     */
    VirtualMedia(CrowApp &app) :
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 2)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string &name = params[0];
        const std::string &resName = params[1];

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (name != "bmc")
        {
            messages::resourceNotFound(asyncResp->res, "VirtualMedia", resName);

            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, name, resName](const boost::system::error_code ec,
                                       const GetObjectType &getObjectType) {
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
            "/xyz/openbmc_project/VirtualMedia", std::array<const char *, 0>());
    }
};

} // namespace redfish
