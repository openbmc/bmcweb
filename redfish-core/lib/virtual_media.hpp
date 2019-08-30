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

namespace redfish

{

/**
 * @brief Get $expand parameter from url, '.' and '*' as true as underlying
 * item has no further nesting - also levels argument is ignored
 */
inline bool getExpandParam(crow::Response &res, const crow::Request &req,
                           bool &expand)
{
    bool valid = false;

    char *expandParam = req.urlParams.get("$expand");
    if (expandParam != nullptr)
    {
        if (*expandParam == '.' || *expandParam == '*')
        {
            // test next character(s)
            expandParam++;
            if (*expandParam == '\0')
            {
                BMCWEB_LOG_DEBUG << "Single char";
                expand = true;
                valid = true;
            }
            if (strcmp(expandParam, "($levels=") == 0)
            {
                // ignoring levels as no further nesting is possible
                BMCWEB_LOG_DEBUG << "Levels specified";
                expand = true;
                valid = true;
            }
        }
        else if (*expandParam == '~')
        {
            // VM item does not contain links
            expand = false;
            valid = true;
        }
    }

    if (!valid)
    {
        messages::queryParameterValueFormatError(
            res, std::string(1, *expandParam), "$expand");
        return false;
    }

    return true;
}

/**
 * @brief Read all known properties from VM object interfaces
 */
static void vmParseInterfaceObject(const DbusInterfaceType &interface,
                                   nlohmann::json &json)
{
    for (const auto &interface : interface)
    {
        if (interface.first == "xyz.openbmc_project.VirtualMedia.MountPoint")
        {
            for (const auto &property : interface.second)
            {
                if (property.first == "EndpointId")
                {
                    const std::string *value =
                        std::get_if<std::string>(&property.second);
                    if (value)
                    {
                        json["Oem"]["WebSocketEndpoint"] = *value;
                    }
                }
            }
        }
        else if (interface.first == "xyz.openbmc_project.VirtualMedia.Process")
        {
            for (const auto &property : interface.second)
            {
                if (property.first == "Active")
                {
                    const bool *value = std::get_if<bool>(&property.second);
                    if (value)
                    {
                        json["Inserted"] = *value;
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
    item["ConnectedVia"] = "Applet";
    item["MediaTypes"] = {"CD", "USBStick"};
    item["TransferMethod"] = "Stream";
    item["TransferProtocolType"] = "OEM";
    item["Oem"]["WebSocketEndpoint"] = nullptr;

    return item;
}

/**
 *  @brief Fills collection data, also with support for $expand
 */
static void getVmResourceList(std::shared_ptr<AsyncResp> aResp,
                              const std::string &name)
{
    BMCWEB_LOG_DEBUG << "Get available Virtual Media resources.";
    crow::connections::systemBus->async_method_call(
        [name, aResp{std::move(aResp)}](const boost::system::error_code ec,
                                        ManagedObjectType &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
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
                else
                {
                    lastIndex += 1;
                }
                item["@odata.id"] = "/redfish/v1/Managers/" + name +
                                    "/VirtualMedia/" + path.substr(lastIndex);

                members.push_back(item);
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.VirtualMedia", "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

/**
 *  @brief Fills data for specific resource
 */
static void getVmData(std::shared_ptr<AsyncResp> aResp, const std::string &name,
                      const std::string &resName)
{
    BMCWEB_LOG_DEBUG << "Get Virtual Media resource data.";

    crow::connections::systemBus->async_method_call(
        [resName, name, aResp](const boost::system::error_code ec,
                               ManagedObjectType &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
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

                vmParseInterfaceObject(item.second, aResp->res.jsonValue);

                return;
            }

            messages::resourceNotFound(
                aResp->res, "#VirtualMedia.v1_1_0.VirtualMedia", resName);
        },
        "xyz.openbmc_project.VirtualMedia", "/xyz/openbmc_project/VirtualMedia",
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

        res.jsonValue["@odata.type"] =
            "#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["Name"] = "Virtual Media Services";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/" + name + "/VirtualMedia/";

        getVmResourceList(asyncResp, name);
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

        getVmData(asyncResp, name, resName);
    }
};

} // namespace redfish
