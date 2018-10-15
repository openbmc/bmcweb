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

#include "node.hpp"

#include <boost/container/flat_map.hpp>

namespace redfish
{

/**
 * DBus types primitives for several generic DBus interfaces
 * TODO(Pawel) consider move this to separate file into boost::dbus
 */
// Note, this is not a very useful Variant, but because it isn't used to get
// values, it should be as simple as possible
// TODO(ed) invent a nullvariant type
using VariantType = sdbusplus::message::variant<bool, std::string, uint64_t>;
using ManagedObjectsType = std::vector<std::pair<
    sdbusplus::message::object_path,
    std::vector<std::pair<std::string,
                          std::vector<std::pair<std::string, VariantType>>>>>>;

using PropertiesType = boost::container::flat_map<std::string, VariantType>;

/**
 * ChassisCollection derived class for delivering Chassis Collection Schema
 */
class ChassisCollection : public Node
{
  public:
    ChassisCollection(CrowApp &app) : Node(app, "/redfish/v1/Chassis/")
    {
        Node::json["@odata.type"] = "#ChassisCollection.ChassisCollection";
        Node::json["@odata.id"] = "/redfish/v1/Chassis";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#ChassisCollection.ChassisCollection";
        Node::json["Name"] = "Chassis Collection";

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
        const std::array<const char *, 4> interfaces = {
            "xyz.openbmc_project.Inventory.Item.Board",
            "xyz.openbmc_project.Inventory.Item.Chassis",
            "xyz.openbmc_project.Inventory.Item.PowerSupply",
            "xyz.openbmc_project.Inventory.Item.System",
        };
        res.jsonValue = Node::json;
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        const std::vector<std::string> &chassisList) {
                if (ec)
                {
                    messages::addMessageToErrorJson(asyncResp->res.jsonValue,
                                                    messages::internalError());
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
                nlohmann::json &chassisArray =
                    asyncResp->res.jsonValue["Members"];
                chassisArray = nlohmann::json::array();
                for (const std::string &objpath : chassisList)
                {
                    std::size_t lastPos = objpath.rfind("/");
                    if (lastPos == std::string::npos)
                    {
                        BMCWEB_LOG_ERROR << "Failed to find '/' in " << objpath;
                        continue;
                    }
                    chassisArray.push_back(
                        {{"@odata.id", "/redfish/v1/Chassis/" +
                                           objpath.substr(lastPos + 1)}});
                }

                asyncResp->res.jsonValue["Members@odata.count"] =
                    chassisArray.size();
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
            "/xyz/openbmc_project/inventory", int32_t(3), interfaces);
    }
};

/**
 * Chassis override class for delivering Chassis Schema
 */
class Chassis : public Node
{
  public:
    Chassis(CrowApp &app) :
        Node(app, "/redfish/v1/Chassis/<str>/", std::string())
    {
        Node::json["@odata.type"] = "#Chassis.v1_4_0.Chassis";
        Node::json["@odata.id"] = "/redfish/v1/Chassis";
        Node::json["@odata.context"] = "/redfish/v1/$metadata#Chassis.Chassis";
        Node::json["Name"] = "Chassis Collection";
        Node::json["ChassisType"] = "RackMount";

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
        // Check if there is required param, truly entering this shall be
        // impossible.
        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.end();
            return;
        }

        res.jsonValue = Node::json;
        const std::string &chassisId = params[0];
        auto asyncResp = std::make_shared<AsyncResp>(res);
        crow::connections::systemBus->async_method_call(
            [asyncResp, chassisId(std::string(chassisId))](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                if (ec)
                {
                    messages::addMessageToErrorJson(asyncResp->res.jsonValue,
                                                    messages::internalError());
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
                // Iterate over all retrieved ObjectPaths.
                for (const std::pair<
                         std::string,
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>
                         &object : subtree)
                {
                    const std::string &path = object.first;
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>
                        &connectionNames = object.second;

                    if (!boost::ends_with(path, chassisId))
                    {
                        continue;
                    }
                    if (connectionNames.size() < 1)
                    {
                        BMCWEB_LOG_ERROR << "Only got "
                                         << connectionNames.size()
                                         << " Connection names";
                        continue;
                    }

                    const std::string connectionName = connectionNames[0].first;
                    crow::connections::systemBus->async_method_call(
                        [asyncResp, chassisId(std::string(chassisId))](
                            const boost::system::error_code ec,
                            const std::vector<std::pair<
                                std::string, VariantType>> &propertiesList) {
                            for (const std::pair<std::string, VariantType>
                                     &property : propertiesList)
                            {
                                const std::string *value =
                                    mapbox::getPtr<const std::string>(
                                        property.second);
                                if (value != nullptr)
                                {
                                    asyncResp->res.jsonValue[property.first] =
                                        *value;
                                }
                            }
                            asyncResp->res.jsonValue["Name"] = chassisId;
                            asyncResp->res.jsonValue["Id"] = chassisId;
                            asyncResp->res.jsonValue["Thermal"] = {
                                {"@odata.id", "/redfish/v1/Chassis/" +
                                                  chassisId + "/Thermal"}};
                        },
                        connectionName, path, "org.freedesktop.DBus.Properties",
                        "GetAll",
                        "xyz.openbmc_project.Inventory.Decorator.Asset");
                    return;
                }

                // Couldn't find an object with that name.  return an error
                asyncResp->res.jsonValue = redfish::messages::resourceNotFound(
                    "#Chassis.v1_4_0.Chassis", chassisId);
                asyncResp->res.result(boost::beast::http::status::not_found);
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/inventory", int32_t(0),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Inventory.Decorator.Asset"});
    }
};
} // namespace redfish
