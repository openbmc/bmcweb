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
static std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;

class UpdateService : public Node
{
  public:
    UpdateService(CrowApp &app) : Node(app, "/redfish/v1/UpdateService/")
    {
        Node::json["@odata.type"] = "#UpdateService.v1_2_0.UpdateService";
        Node::json["@odata.id"] = "/redfish/v1/UpdateService";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#UpdateService.UpdateService";
        Node::json["Id"] = "UpdateService";
        Node::json["Description"] = "Service for Software Update";
        Node::json["Name"] = "Update Service";
        Node::json["HttpPushUri"] = "/redfish/v1/UpdateService";
        // UpdateService cannot be disabled
        Node::json["ServiceEnabled"] = true;
        Node::json["FirmwareInventory"] = {
            {"@odata.id", "/redfish/v1/UpdateService/FirmwareInventory"}};

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue = Node::json;
        res.end();
    }
    static void activateImage(const std::string &objPath)
    {
        crow::connections::systemBus->async_method_call(
            [objPath](const boost::system::error_code error_code) {
                if (error_code)
                {
                    BMCWEB_LOG_DEBUG << "error_code = " << error_code;
                    BMCWEB_LOG_DEBUG << "error msg = " << error_code.message();
                }
            },
            "xyz.openbmc_project.Software.BMC.Updater", objPath,
            "org.freedesktop.DBus.Properties", "Set",
            "xyz.openbmc_project.Software.Activation", "RequestedActivation",
            sdbusplus::message::variant<std::string>(
                "xyz.openbmc_project.Software.Activation.RequestedActivations."
                "Active"));
    }
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "doPost...";

        // Only allow one FW update at a time
        if (fwUpdateMatcher != nullptr)
        {
            res.addHeader("Retry-After", "30");
            res.result(boost::beast::http::status::service_unavailable);
            res.jsonValue = messages::serviceTemporarilyUnavailable("3");
            res.end();
            return;
        }
        // Make this const static so it survives outside this method
        static boost::asio::deadline_timer timeout(
            *req.ioService, boost::posix_time::seconds(5));

        timeout.expires_from_now(boost::posix_time::seconds(5));

        timeout.async_wait([&res](const boost::system::error_code &ec) {
            fwUpdateMatcher = nullptr;
            if (ec == boost::asio::error::operation_aborted)
            {
                // expected, we were canceled before the timer completed.
                return;
            }
            BMCWEB_LOG_ERROR
                << "Timed out waiting for firmware object being created";
            BMCWEB_LOG_ERROR
                << "FW image may has already been uploaded to server";
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Async_wait failed" << ec;
                return;
            }

            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue = redfish::messages::internalError();
            res.end();
        });

        auto callback = [&res](sdbusplus::message::message &m) {
            BMCWEB_LOG_DEBUG << "Match fired";
            bool flag = false;

            if (m.is_method_error())
            {
                BMCWEB_LOG_DEBUG << "Dbus method error!!!";
                res.end();
                return;
            }
            std::vector<std::pair<
                std::string,
                std::vector<std::pair<
                    std::string, sdbusplus::message::variant<std::string>>>>>
                interfaces_properties;

            sdbusplus::message::object_path objPath;

            m.read(objPath, interfaces_properties); // Read in the object path
                                                    // that was just created
            // std::string str_objpath = objPath.str;  // keep a copy for
            // constructing response message
            BMCWEB_LOG_DEBUG << "obj path = " << objPath.str; // str_objpath;
            for (auto &interface : interfaces_properties)
            {
                BMCWEB_LOG_DEBUG << "interface = " << interface.first;

                if (interface.first ==
                    "xyz.openbmc_project.Software.Activation")
                {
                    // cancel timer only when
                    // xyz.openbmc_project.Software.Activation interface is
                    // added
                    boost::system::error_code ec;
                    timeout.cancel(ec);
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "error canceling timer " << ec;
                    }
                    UpdateService::activateImage(objPath.str); // str_objpath);
                    res.jsonValue = redfish::messages::success();
                    BMCWEB_LOG_DEBUG << "ending response";
                    res.end();
                    fwUpdateMatcher = nullptr;
                }
            }
        };

        fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
            *crow::connections::systemBus,
            "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
            "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
            callback);

        std::string filepath(
            "/tmp/images/" +
            boost::uuids::to_string(boost::uuids::random_generator()()));
        BMCWEB_LOG_DEBUG << "Writing file to " << filepath;
        std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                        std::ofstream::trunc);
        out << req.body;
        out.close();
        BMCWEB_LOG_DEBUG << "file upload complete!!";
    }
};

class SoftwareInventoryCollection : public Node
{
  public:
    template <typename CrowApp>
    SoftwareInventoryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/UpdateService/FirmwareInventory/")
    {
        Node::json["@odata.type"] =
            "#SoftwareInventoryCollection.SoftwareInventoryCollection";
        Node::json["@odata.id"] = "/redfish/v1/UpdateService/FirmwareInventory";
        Node::json["@odata.context"] =
            "/redfish/v1/"
            "$metadata#SoftwareInventoryCollection.SoftwareInventoryCollection";
        Node::json["Name"] = "Software Inventory Collection";

        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = Node::json;

        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                if (ec)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                for (auto &obj : subtree)
                {
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>
                        &connections = obj.second;

                    for (auto &conn : connections)
                    {
                        const std::string &connectionName = conn.first;
                        BMCWEB_LOG_DEBUG << "connectionName = "
                                         << connectionName;
                        BMCWEB_LOG_DEBUG << "obj.first = " << obj.first;

                        crow::connections::systemBus->async_method_call(
                            [asyncResp](
                                const boost::system::error_code error_code,
                                const VariantType &activation) {
                                BMCWEB_LOG_DEBUG
                                    << "safe returned in lambda function";
                                if (error_code)
                                {
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }

                                const std::string *sw_inv_purpose =
                                    mapbox::getPtr<const std::string>(
                                        activation);
                                if (sw_inv_purpose == nullptr)
                                {
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }
                                std::size_t last_pos =
                                    sw_inv_purpose->rfind(".");
                                if (last_pos == std::string::npos)
                                {
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }
                                nlohmann::json &members =
                                    asyncResp->res.jsonValue["Members"];
                                members.push_back(
                                    {{"@odata.id", "/redfish/v1/UpdateService/"
                                                   "FirmwareInventory/" +
                                                       sw_inv_purpose->substr(
                                                           last_pos + 1)}});
                                asyncResp->res
                                    .jsonValue["Members@odata.count"] =
                                    members.size();
                            },
                            connectionName, obj.first,
                            "org.freedesktop.DBus.Properties", "Get",
                            "xyz.openbmc_project.Software.Activation",
                            "Activation");
                    }
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/software", int32_t(1),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Software.Version"});
    }
};

class SoftwareInventory : public Node
{
  public:
    template <typename CrowApp>
    SoftwareInventory(CrowApp &app) :
        Node(app, "/redfish/v1/UpdateService/FirmwareInventory/<str>/",
             std::string())
    {
        Node::json["@odata.type"] =
            "#SoftwareInventory.v1_1_0.SoftwareInventory";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory";
        Node::json["Name"] = "Software Inventory";
        Node::json["Updateable"] = false;
        Node::json["Status"]["Health"] = "OK";
        Node::json["Status"]["HealthRollup"] = "OK";
        Node::json["Status"]["State"] = "Enabled";
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue = Node::json;

        if (params.size() != 1)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue = messages::internalError();
            res.end();
            return;
        }

        std::shared_ptr<std::string> sw_id =
            std::make_shared<std::string>(params[0]);

        res.jsonValue["@odata.id"] =
            "/redfish/v1/UpdateService/FirmwareInventory/" + *sw_id;

        crow::connections::systemBus->async_method_call(
            [asyncResp, sw_id](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                BMCWEB_LOG_DEBUG << "doGet callback...";
                if (ec)
                {
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }

                for (const std::pair<
                         std::string,
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>
                         &obj : subtree)
                {
                    if (boost::ends_with(obj.first, *sw_id) != true)
                    {
                        continue;
                    }

                    if (obj.second.size() <= 1)
                    {
                        continue;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp,
                         sw_id](const boost::system::error_code error_code,
                                const boost::container::flat_map<
                                    std::string, VariantType> &propertiesList) {
                            if (error_code)
                            {
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
                                return;
                            }
                            boost::container::flat_map<
                                std::string, VariantType>::const_iterator it =
                                propertiesList.find("Purpose");
                            if (it == propertiesList.end())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Can't find property \"Purpose\"!";
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
                                return;
                            }
                            const std::string *sw_inv_purpose =
                                mapbox::getPtr<const std::string>(it->second);
                            if (sw_inv_purpose == nullptr)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "wrong types for property\"Purpose\"!";
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
                                return;
                            }

                            BMCWEB_LOG_DEBUG << "sw_inv_purpose = "
                                             << *sw_inv_purpose;
                            if (boost::ends_with(*sw_inv_purpose, "." + *sw_id))
                            {
                                it = propertiesList.find("Version");
                                if (it == propertiesList.end())
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Can't find property \"Version\"!";
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }

                                const std::string *version =
                                    mapbox::getPtr<const std::string>(
                                        it->second);

                                if (version != nullptr)
                                {
                                    BMCWEB_LOG_DEBUG
                                        << "Can't find property \"Version\"!";
                                    asyncResp->res.result(
                                        boost::beast::http::status::
                                            internal_server_error);
                                    return;
                                }
                                asyncResp->res.jsonValue["Version"] = *version;
                                asyncResp->res.jsonValue["Id"] = *sw_id;
                            }
                        },
                        obj.second[0].first, obj.first,
                        "org.freedesktop.DBus.Properties", "GetAll",
                        "xyz.openbmc_project.Software.Version");
                }
            },
            "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/software", int32_t(1),
            std::array<const char *, 1>{
                "xyz.openbmc_project.Software.Version"});
    }
};

} // namespace redfish
