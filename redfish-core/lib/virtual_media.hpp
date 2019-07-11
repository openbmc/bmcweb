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
#include <../lib/account_service.hpp>

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
            aResp->res.jsonValue["Oem"]["WebSocketEndpoint"] = *endpointIdValue;
            aResp->res.jsonValue["TransferProtocolType"] = "OEM";
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
    item["@odata.type"] = "#VirtualMedia.v1_1_0.VirtualMedia";
    item["@odata.context"] = "/redfish/v1/$metadata#VirtualMedia.VirtualMedia";
    item["Name"] = "Virtual Removable Media";
    item["Id"] = resName;
    item["Image"] = nullptr;
    item["ImageName"] = nullptr;
    item["WriteProtected"] = true;
    item["ConnectedVia"] = "NotConnected";
    item["MediaTypes"] = {"CD", "USBStick"};
    item["TransferMethod"] = "Stream";
    item["TransferProtocolType"] = nullptr;
    item["Oem"]["WebSocketEndpoint"] = nullptr;

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

                vmParseInterfaceObject(item.second, aResp);

                return;
            }

            messages::resourceNotFound(
                aResp->res, "#VirtualMedia.v1_1_0.VirtualMedia", resName);
        },
        service, "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

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
