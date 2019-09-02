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

		// Check if dbus path is Legacy type
		if (path.find("VirtualMedia/Legacy") != std::string::npos)
		{
		    aResp->res.jsonValue["Actions"]["#VirtualMedia.InsertMedia"]["target"] =
			"/redfish/v1/Managers/" + name + "/VirtualMedia/" + resName +
			"/Actions/VirtualMedia.InsertMedia";
		}

                vmParseInterfaceObject(item.second, aResp->res.jsonValue);

                return;
            }

            messages::resourceNotFound(
                aResp->res, "#VirtualMedia.v1_1_0.VirtualMedia", resName);
        },
        "xyz.openbmc_project.VirtualMedia", "/xyz/openbmc_project/VirtualMedia",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

class VirtualMediaActions : public Node
{
  public:
    VirtualMediaActions(CrowApp &app, const std::string vmAction,
                        const std::string dBusAction) :
        Node(app,
             "/redfish/v1/Managers/<str>/VirtualMedia/<str>/Actions/"
             "VirtualMedia." +
                 vmAction,
             std::string(), std::string()),
        vmAction(vmAction), dBusAction(dBusAction)
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
     * @brief Function handles GET method request.
     *
     * it is not required to retrieve more information in GET.
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.end();
    }

    /**
     * @brief Function handles POST method request.
     *
     * Analyzes POST body message before sends Reset request data to dbus.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
	std::string resName;

        if (params.size() != 2)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        // take resource name from URL
        resName = params[1];

        auto aResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [this, resName, req,
             aResp{std::move(aResp)}](const boost::system::error_code ec,
                                      VmManagedObjectType &subtree) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG << "DBUS response error";
                    messages::internalError(aResp->res);
                    return;
                }

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

                    if (path.substr(lastIndex) == resName)
                    {
			lastIndex = path.rfind("Proxy");
			if (lastIndex != std::string::npos)
			{
			    if (vmAction == "EjectMedia")
			    {
				doVmAction(aResp->res, req, resName, false, "");
			    }
			    else
			    {
				aResp->res.result(boost::beast::http::status::not_found);
				aResp->res.end();
				return;
			    }
			}
                        lastIndex = path.rfind("Legacy");
                        if (lastIndex != std::string::npos)
                        {
                            // Legacy mode
                            std::string imageUrl;

                            // Read obligatory paramters (url of image)
                            if (!json_util::readJson(req, aResp->res, "Image",
                                                     imageUrl))
                            {
                                return;
                            }

                            // must not be empty
                            if (imageUrl.size() == 0)
                            {
                                aResp->res.result(
                                    boost::beast::http::status::bad_request);
                                messages::actionParameterValueFormatError(
                                    aResp->res, "\"\"", "Image", vmAction);
                                BMCWEB_LOG_ERROR << "Request action parameter "
                                                    "Image is empty.";
                                aResp->res.end();
                                return;
                            }

                            // manager is irrelevant for VirtualMedia dbus calls
                            doVmAction(aResp->res, req, resName, true, imageUrl);
                        }
                    }
                }
            },
            "xyz.openbmc_project.VirtualMedia",
            "/xyz/openbmc_project/VirtualMedia",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }

    /**
     * @brief Function transceives data with dbus directly.
     *
     * All BMC state properties will be retrieved before sending reset request.
     */
    void doVmAction(crow::Response &res, const crow::Request &req,
                    const std::string &name, bool legacy, const std::string &imageUrl)
    {
        // Create the D-Bus variant for D-Bus call.
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

	// Legacy mount requires parameter with image
	if (legacy && dBusAction == "Mount")
	{
	    crow::connections::systemBus->async_method_call(
		[asyncResp](const boost::system::error_code ec) {
		    if (ec)
		    {
			BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;
			asyncResp->res.result(
			    boost::beast::http::status::internal_server_error);
			messages::internalError(asyncResp->res);
			return;
		    }

		    asyncResp->res.result(boost::beast::http::status::ok);
		    messages::success(asyncResp->res);
		},
		"xyz.openbmc_project.VirtualMedia",
		"/xyz/openbmc_project/VirtualMedia/Legacy/" + name,
		"xyz.openbmc_project.VirtualMedia.Legacy", dBusAction, imageUrl);
	}
	else // proxy mount and umount
	{
	    crow::connections::systemBus->async_method_call(
		[asyncResp](const boost::system::error_code ec) {
		    if (ec)
		    {
			BMCWEB_LOG_ERROR << "Bad D-Bus request error: " << ec;
			asyncResp->res.result(
			    boost::beast::http::status::internal_server_error);
			messages::internalError(asyncResp->res);
			return;
		    }

		    asyncResp->res.result(boost::beast::http::status::ok);
		    messages::success(asyncResp->res);
		},
		"xyz.openbmc_project.VirtualMedia",
		"/xyz/openbmc_project/VirtualMedia/Proxy/" + name,
		"xyz.openbmc_project.VirtualMedia.Proxy", dBusAction);
	}
    }

    /** Action name in redfish */
    std::string vmAction;
    /** Action name in DBus */
    std::string dBusAction;
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

        res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"]["target"] =
            "/redfish/v1/Managers/" + name + "/VirtualMedia/" + resName +
            "/Actions/VirtualMedia.EjectMedia";
    }
};

} // namespace redfish
