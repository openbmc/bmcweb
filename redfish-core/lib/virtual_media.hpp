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
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <error_messages.hpp>
#include <node.hpp>
#include <optional>
#include <utils/json_utils.hpp>
#include <variant>

namespace redfish
{

/**
 *  @DBus Interfaces and Services of Virtual Media
 */
namespace virtualmedia
{
constexpr char const *virtualmediaObjectPath = "/xyz/openbmc_project/VirtualMedia/Legacy";
constexpr char const *LegacyIntf = "xyz.openbmc_project.VirtualMedia.Legacy";
constexpr char const *MountPointIntf = "xyz.openbmc_project.VirtualMedia.MountPoint";
constexpr char const *dbusPropIntf = "org.freedesktop.DBus.Properties";
constexpr char const *dbusObjManagerIntf = "org.freedesktop.DBus.ObjectManager";
constexpr char const *virtualmedia_ObjectPath = "/xyz/openbmc_project/VirtualMedia";
constexpr char const *virtualmediaServiceName =
			"xyz.openbmc_project.VirtualMedia";
constexpr char const *virtual_media_user_dir = "/etc/vmedia_user_credentials";
} //namespace virtualmedia


/**
 * @brief Set the requested UserName or Password in file
 * @param[in] req_usernameorpassword UserName or Password to be set
 *
 * @return true on success or flase on failture
 */
bool setUserNameorPassword(const std::string& req_usernameorpassword)
{
        std::ofstream fileWriter;
        fileWriter.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	size_t ret = req_usernameorpassword.find("username=");
	if(ret != std::string::npos)
	{
           fileWriter.open( redfish::virtualmedia::virtual_media_user_dir, std::ios::out | std::ios::trunc);
	}
	else if((ret = req_usernameorpassword.find("password=")) != std::string::npos)
	{
           fileWriter.open(redfish::virtualmedia::virtual_media_user_dir, std::ios::out | std::ios::app);
	   fileWriter.seekp(0,std::ios::end);
	}
        fileWriter << req_usernameorpassword << std::endl; // Make sure for new line and flush
        fileWriter.close();
	return true;
}

/**
 * @brief Get the username from the file, which is already set by POST call
 * @param[in] no parameters required
 *
 * @return UserName if present else returns empty string
 */
std::string getUserName(void)
{
    std::ifstream fileWriter;
    fileWriter.open(redfish::virtualmedia::virtual_media_user_dir);
    const std::string temp_buf = "username=";  
    static std::string username;  

    if(fileWriter)
    {
         while(std::getline(fileWriter,username))
         {
            if(username.find(temp_buf) != std::string::npos)
            {
	       size_t len = temp_buf.length();
	       username = username.substr(len);
               return username;
            }
         }
    }
    return std::string("");
}


/**
 * VirtualMedia Collection derived class for delivering Virtual Media Schema
 */
class VirtualMediaCollection : public Node
{
  public:
    template <typename CrowApp>
    VirtualMediaCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] =
            "#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#VirtualMediaCollection.VirtualMediaCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/VirtualMedia";
        res.jsonValue["Name"] = "Virtual Media Collection";
        res.jsonValue["Description"] =
            "Collection of Virtual Media redirected to Host via this Manager";

        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        // Get Virtual Media interface list, and call the below callback for JSON
        // preparation
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                nlohmann::json& memberArray =
                    asyncResp->res.jsonValue["Members"];
                memberArray = nlohmann::json::array();
	
		std::size_t InstanceCount = 0;
                for (auto& instance : instances)
                {
                    const std::string& path =
                        static_cast<const std::string&>(instance.first);
		    
		    //Check Only for the Legacy Object Path
                    std::size_t lastIndex = path.find("Legacy");
		    if (lastIndex != std::string::npos)
		    {
                    	lastIndex = path.rfind("/");
                    	memberArray.push_back(
                        	{{"@odata.id", "/redfish/v1/Managers/bmc/VirtualMedia/" +
                                           	path.substr(lastIndex + 1)}});
			InstanceCount += 1;
		    }
                }
                asyncResp->res.jsonValue["Members@odata.count"] = InstanceCount;
            },
            redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
            redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");
    }
};


/**
 * VirtualMedia Instance derived class for delivering Virtual Media Schema
 */
class VirtualMediaInstance : public Node
{
  public:
    template <typename CrowApp>
    VirtualMediaInstance(CrowApp& app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/<str>/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }


  private:

    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {

        auto asyncResp = std::make_shared<AsyncResp>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        crow::connections::systemBus->async_method_call(
            [asyncResp, instanceName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto instanceIt = instances.begin();
                for (; instanceIt != instances.end(); instanceIt++)
                {
                    if (boost::ends_with(instanceIt->first.str, "/" + instanceName))
                    {
                        break;
                    }
                }

                if (instanceIt == instances.end())
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMediaInstance",
                                               instanceName);
                    return;
                }

                asyncResp->res.jsonValue = {
                    {"@odata.context",
                     "/redfish/v1/$metadata#VirtualMedia.VirtualMedia"},
                    {"@odata.type", "#VirtualMedia.v1_3_1.VirtualMedia"},
                    {"Name", "Virtual Media Interface " + instanceName},
                    {"Description", "Virtual Media Instance redirected to host via this Manager"},
                    {"TransferMethod", "Stream"},
                    {"ConnectedVia", "URI"}};

		std::string UserName = "";
		/* Check for ImageUrl Property Status */
                for (const auto& interface : instanceIt->second)
                {
                    if (interface.first ==
                        "xyz.openbmc_project.VirtualMedia.MountPoint")
                    {
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "ImageUrl")
                            {
                                const std::string* imageUrl =
				    std::get_if<std::string>(&property.second);

				/* Check if ImageUrl Property Set or Not */
                                const std::string imageName = *imageUrl;
				std::size_t lastIndex = imageName.rfind("/");
				asyncResp->res.jsonValue["ImageName"] = imageName.substr(lastIndex + 1);
				if (!imageName.empty())
				{
					lastIndex = imageName.find("//");
                                	asyncResp->res.jsonValue["Image"] = imageName.substr(lastIndex);
					UserName = getUserName();
					asyncResp->res.jsonValue["Inserted"] =true;

					lastIndex = std::string::npos;
                                        if((lastIndex = imageName.find("smb")) != std::string::npos)
                                        {
                                           asyncResp->res.jsonValue["TransferProtocolType"] ="CIFS";
                                        }
                                        else if((lastIndex = imageName.find("https")) != std::string::npos)
                                        {
                                           asyncResp->res.jsonValue["TransferProtocolType"] ="HTTPS";
                                        }
				}
				else
				{
					asyncResp->res.jsonValue["Image"] = *imageUrl;
					asyncResp->res.jsonValue["WriteProtected"] = true;
					asyncResp->res.jsonValue["Inserted"] =false;
					asyncResp->res.jsonValue["TransferProtocolType"] ="CIFS";
				}
			    }
		
                        }
				
                    }
                }
	
		asyncResp->res.jsonValue["UserName"] = UserName;
		asyncResp->res.jsonValue["MediaTypes"] = { "CD" ,
							   "USBStick"};
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/VirtualMedia/" + instanceName;
                asyncResp->res.jsonValue["Id"] = instanceName;

	        asyncResp->res.jsonValue["Actions"]["#VirtualMedia.EjectMedia"] = {
	            {"@Redfish.ActionInfo", "/redfish/v1/Managers/bmc/VirtualMedia/"
			       + instanceName + "/EjectMediaActionInfo"},
        	    {"target", "/redfish/v1/Managers/bmc/VirtualMedia/"
                	       + instanceName + "/Actions/VirtualMedia.EjectMedia"}};
	        asyncResp->res.jsonValue["Actions"]["#VirtualMedia.InsertMedia"] = {
	            {"@Redfish.ActionInfo", "/redfish/v1/Managers/bmc/VirtualMedia/"
			       + instanceName + "/InsertMediaActionInfo"},
        	    {"target", "/redfish/v1/Managers/bmc/VirtualMedia/"
                	       + instanceName + "/Actions/VirtualMedia.InsertMedia"}};
	
            },
            redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
            redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");
    }
};


/**
 * VirtualMedia InsertMedia ActionInfo derived Class
 * to Get the available actions to perfrom InsertMedia Action
 */
class VirtualMediaInsertMediaActionInfo : public Node
{
  public:
    VirtualMediaInsertMediaActionInfo(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/<str>/InsertMediaActionInfo/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }

	std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.type"] =
            "#ActionInfo.v1_1_1.ActionInfo";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#ActionInfo.ActionInfo";
        asyncResp->res.jsonValue["Name"] = "InsertMedia";
        asyncResp->res.jsonValue["Id"] = "InsertMedia";
        asyncResp->res.jsonValue["Description"] =
            "This action is used to attach remote media to virtual media";

       crow::connections::systemBus->async_method_call(
            [asyncResp, instanceName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                auto instanceIt = instances.begin();

                for (; instanceIt != instances.end(); instanceIt++)
                {
                    if (boost::ends_with(instanceIt->first.str, "/" + instanceName))
                    {
                        break;
                    }
                }
                if (instanceIt == instances.end())
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMediaInstance",
                                               instanceName);
                    return;
                }
	
	        asyncResp->res.jsonValue["@odata.id"] =
	            "/redfish/v1/Managers/bmc/VirtualMedia/" + instanceName + "/InsertMediaActionInfo";

		
	        asyncResp->res.jsonValue["Parameters"] = {{{"Datatype","String"},
	                                        {"Name","Image"},
	                                        {"Required",true}},
	                                        {{"Datatype","Boolean"},
	                                        {"Name","Inserted"},
	                                        {"Required",false}},
	                                        {{"Datatype","Boolean"},
	                                        {"Name","WriteProtected"},
	                                        {"Required",false}},
	                                        {{"AllowableValues",{"CIFS",
	                                                             "HTTPS"}},
	                                         {"Datatype","String"},
	                                         {"Name","TransferProtocolType"},
	                                         {"Required",true}},
	                                        {{"AllowableValues",{"Stream"}},
	                                         {"Datatype","String"},
	                                         {"Name","TransferMethod"},
	                                         {"Required",false}},
	                                        {{"Datatype","String"},
	                                        {"Name","UserName"},
	                                        {"Required",true}},
	                                        {{"Datatype","String"},
	                                        {"Name","Password"},
	                                        {"Required",true}},
	                                      };
            },
            redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
            redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");
    }
};


/**
 * VirtualMedia EjectMedia ActionInfo derived Class 
 * to Get the available actions to perfrom EjectMedia Action
 */
class VirtualMediaEjectMediaActionInfo : public Node
{
  public:
    VirtualMediaEjectMediaActionInfo(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/<str>/EjectMediaActionInfo/", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /**
     * Functions triggers appropriate requests on DBus
     */
    void doGet(crow::Response& res, const crow::Request& req,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }
	std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.type"] =
            "#ActionInfo.v1_1_1.ActionInfo";
        asyncResp->res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#ActionInfo.ActionInfo";
        asyncResp->res.jsonValue["Name"] = "EjectMedia";
        asyncResp->res.jsonValue["Id"] = "EjectMedia";
        asyncResp->res.jsonValue["Description"] =
            "This action is used to detach remote media from virtual media";

       crow::connections::systemBus->async_method_call(
            [asyncResp, instanceName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                auto instanceIt = instances.begin();

                for (; instanceIt != instances.end(); instanceIt++)
                {
                    if (boost::ends_with(instanceIt->first.str, "/" + instanceName))
                    {
                        break;
                    }
                }
                if (instanceIt == instances.end())
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMediaInstance",
                                               instanceName);
                    return;
                }
	
	        asyncResp->res.jsonValue["@odata.id"] =
	            "/redfish/v1/Managers/bmc/VirtualMedia/" + instanceName + "/EjectMediaActionInfo";

		/* EjectMedia Action will not take any input,
		 * so no need to show any request data
   		*/

            },
            redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
            redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");

    }
};


/**
 * VirtualMedia InsertMedia Action to Perform InsertMedia action 
 * for redirecting the media device from Client machine to
 * to Host machine via BMC. 
 */
class VirtualMediaInsertMediaAction : public Node
{
  public:
    VirtualMediaInsertMediaAction(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/<str>/Actions/VirtualMedia.InsertMedia", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        // Required parameters
        std::string image;
        std::string transferprotocoltype;
        std::string username;
        std::string password;
	//Option Parameters
	std::string imagepath;
	std::string Instance=std::string(params[0]);

        /**
         * Functions triggers appropriate requests on DBus
         */
        crow::connections::systemBus->async_method_call(
            [asyncResp, instanceName{std::string(params[0])},req](
                const boost::system::error_code ec,
                const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                auto instanceIt = instances.begin();

                for (; instanceIt != instances.end(); instanceIt++)
                {
                    if (boost::ends_with(instanceIt->first.str, "/" + instanceName))
                    {
                        break;
                    }
                }
                if (instanceIt == instances.end())
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMediaInstance",
                                               instanceName);
                    return;
                }

		/* Check if already Image is inserted or not */
                for (const auto& interface : instanceIt->second)
                {
                    if (interface.first ==
                        "xyz.openbmc_project.VirtualMedia.MountPoint")
                    {
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "ImageUrl")
                            {
                                const std::string* imageUrl =
                                    std::get_if<std::string>(&property.second);

				//Check if with the requested instance
				//media is already mounted or free
                                if (!imageUrl->empty())
                                {
				    //returning as instance is already mounted media
				    messages::instanceInUse(asyncResp->res,instanceName);
				    return;
                                }
		            }
			}
		    }
		}
        },
        redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
        redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");
		
	//Reading the requested data
        if (!json_util::readJson(
                req, asyncResp->res, "Image", image, "TransferProtocolType", transferprotocoltype,
		"UserName", username, "Password", password))
        {
            return;
        }

	//Validate the requested data
	size_t startIndex = image.find("//");
	if(((startIndex == std::string::npos) && (startIndex != 0))
		|| ((startIndex = image.rfind(".")) == std::string::npos))
	{
	             messages::propertyValueNotInList(asyncResp->res,
        	                                      image,
                	                              "Image");
        	     return;
	}
	int lastIndex = transferprotocoltype.compare("CIFS");
	if(lastIndex == 0)
	{
	    imagepath = "smb:" + image; 
	}
	else if ((lastIndex = transferprotocoltype.compare("HTTPS")) == 0)
	{
	    imagepath = "https:" + image; 
	}
	else
	{
             messages::propertyValueNotInList(asyncResp->res,
                                              transferprotocoltype,
                                              "TransferProtocolType");
             return;
	}

	/* Set the Requested UserName and Password in file,
	 * which virtual media service will read for mount call
	*/
	auto ret = setUserNameorPassword(std::string("username=") + username);
	if(!ret)
	{
             messages::propertyValueNotInList(asyncResp->res,
                                              transferprotocoltype,
                                              "UserName");
             return;
	}
	ret = setUserNameorPassword(std::string("password=") + password);
	if(!ret)
	{
             messages::propertyValueNotInList(asyncResp->res,
                                              transferprotocoltype,
                                              "Password");
             return;
	}

	std::string InstanceObjectPath = std::string("/xyz/openbmc_project/VirtualMedia/") + "Legacy/" + Instance;

	/* Call the Mount Method */
        crow::connections::systemBus->async_method_call(
            [asyncResp,Instance{std::string(params[0])}](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }

		messages::delayInActionCompletion(asyncResp->res, Instance,
						   "/redfish/v1/Managers/bmc/VirtualMedia" +Instance);
            },
            redfish::virtualmedia::virtualmediaServiceName, InstanceObjectPath,
	    redfish::virtualmedia::LegacyIntf, "Mount", imagepath);
    }

}; // VirtualMediaInsertMediaAction



/**
 * VirtualMedia_EjectMedia_Action to Perform EjectMedia action 
 * to UnMount the media device from the Host machine
 */
class VirtualMediaEjectMediaAction : public Node
{
  public:
    VirtualMediaEjectMediaAction(CrowApp &app) :
        Node(app, "/redfish/v1/Managers/bmc/VirtualMedia/<str>/Actions/VirtualMedia.EjectMedia", std::string())
    {
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
	std::string Instance=std::string(params[0]);

        /**
         * Functions triggers appropriate requests on DBus
         */
        crow::connections::systemBus->async_method_call(
            [asyncResp, instanceName{std::string(params[0])}](
                const boost::system::error_code ec,
                const ManagedObjectType& instances) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                auto instanceIt = instances.begin();
                for (; instanceIt != instances.end(); instanceIt++)
                {
                    if (boost::ends_with(instanceIt->first.str, "/" + instanceName))
                    {
                        break;
                    }
                }
                if (instanceIt == instances.end())
                {
                    messages::resourceNotFound(asyncResp->res, "VirtualMediaInstance",
                                               instanceName);
                    return;
                }
		
		/* Check for the Instance is already running or not */
                for (const auto& interface : instanceIt->second)
                {
                    if (interface.first ==
                        "xyz.openbmc_project.VirtualMedia.MountPoint")
                    {
                        for (const auto& property : interface.second)
                        {
                            if (property.first == "ImageUrl")
                            {
                                const std::string* imageUrl =
                                    std::get_if<std::string>(&property.second);

				/* Check if with the requested instance
				 * media is already mounted or not
				*/
                                if (imageUrl->empty())
                                {
				    /* returning as instance has not been mounted any media */
				    messages::instanceNotInUse(asyncResp->res,instanceName);
				    return;
                                }
		            }
			}
		    }
		}
        },
        redfish::virtualmedia::virtualmediaServiceName, redfish::virtualmedia::virtualmedia_ObjectPath,
        redfish::virtualmedia::dbusObjManagerIntf , "GetManagedObjects");
		
	std::string InstanceObjectPath = std::string("/xyz/openbmc_project/VirtualMedia/") + "Legacy/" + Instance;

	/* Call the Unmount method */
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "DBUS response error: " << ec.message();
                    messages::internalError(asyncResp->res);
                    return;
                }
		messages::noContent(asyncResp->res);
            },
            redfish::virtualmedia::virtualmediaServiceName, InstanceObjectPath,
	    redfish::virtualmedia::LegacyIntf, "Unmount");
    }

}; // VirtualMediaEjectMediaAction

} // namespace redfish
