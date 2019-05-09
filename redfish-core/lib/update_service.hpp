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
#include <utils/fw_utils.hpp>
#include <variant>

namespace redfish
{

// Match signals added on software path
static std::unique_ptr<sdbusplus::bus::match::match> fwUpdateMatcher;
// Only allow one update at a time
static bool fwUpdateInProgress = false;
// Timer for software available
static std::unique_ptr<boost::asio::deadline_timer> fwAvailableTimer;

static void cleanUp()
{
    fwUpdateInProgress = false;
    fwUpdateMatcher = nullptr;
}
static void activateImage(const std::string &objPath,
                          const std::string &service)
{
    BMCWEB_LOG_DEBUG << "Activate image for " << objPath << " " << service;
    crow::connections::systemBus->async_method_call(
        [](const boost::system::error_code error_code) {
            if (error_code)
            {
                BMCWEB_LOG_DEBUG << "error_code = " << error_code;
                BMCWEB_LOG_DEBUG << "error msg = " << error_code.message();
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "Set",
        "xyz.openbmc_project.Software.Activation", "RequestedActivation",
        std::variant<std::string>(
            "xyz.openbmc_project.Software.Activation.RequestedActivations."
            "Active"));
}
static void softwareInterfaceAdded(std::shared_ptr<AsyncResp> asyncResp,
                                   sdbusplus::message::message &m)
{
    std::vector<std::pair<
        std::string,
        std::vector<std::pair<std::string, std::variant<std::string>>>>>
        interfacesProperties;

    sdbusplus::message::object_path objPath;

    m.read(objPath, interfacesProperties);

    BMCWEB_LOG_DEBUG << "obj path = " << objPath.str;
    for (auto &interface : interfacesProperties)
    {
        BMCWEB_LOG_DEBUG << "interface = " << interface.first;

        if (interface.first == "xyz.openbmc_project.Software.Activation")
        {
            // Found our interface, disable callbacks
            fwUpdateMatcher = nullptr;

            // Retrieve service and activate
            crow::connections::systemBus->async_method_call(
                [objPath, asyncResp](
                    const boost::system::error_code error_code,
                    const std::vector<std::pair<
                        std::string, std::vector<std::string>>> &objInfo) {
                    if (error_code)
                    {
                        BMCWEB_LOG_DEBUG << "error_code = " << error_code;
                        BMCWEB_LOG_DEBUG << "error msg = "
                                         << error_code.message();
                        messages::internalError(asyncResp->res);
                        cleanUp();
                        return;
                    }
                    // Ensure we only got one service back
                    if (objInfo.size() != 1)
                    {
                        BMCWEB_LOG_ERROR << "Invalid Object Size "
                                         << objInfo.size();
                        messages::internalError(asyncResp->res);
                        cleanUp();
                        return;
                    }
                    // cancel timer only when
                    // xyz.openbmc_project.Software.Activation interface
                    // is added
                    fwAvailableTimer = nullptr;

                    activateImage(objPath.str, objInfo[0].first);
                    redfish::messages::success(asyncResp->res);
                    fwUpdateInProgress = false;
                },
                "xyz.openbmc_project.ObjectMapper",
                "/xyz/openbmc_project/object_mapper",
                "xyz.openbmc_project.ObjectMapper", "GetObject", objPath.str,
                std::array<const char *, 1>{
                    "xyz.openbmc_project.Software.Activation"});
        }
    }
}

static void monitorForSoftwareAvailable(std::shared_ptr<AsyncResp> asyncResp,
                                        const crow::Request &req)
{
    // Only allow one FW update at a time
    if (fwUpdateInProgress != false)
    {
        asyncResp->res.addHeader("Retry-After", "30");
        messages::serviceTemporarilyUnavailable(asyncResp->res, "30");
        return;
    }

    fwAvailableTimer = std::make_unique<boost::asio::deadline_timer>(
        *req.ioService, boost::posix_time::seconds(5));

    fwAvailableTimer->expires_from_now(boost::posix_time::seconds(5));

    fwAvailableTimer->async_wait(
        [asyncResp](const boost::system::error_code &ec) {
            cleanUp();
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

            redfish::messages::internalError(asyncResp->res);
        });

    auto callback = [asyncResp](sdbusplus::message::message &m) {
        BMCWEB_LOG_DEBUG << "Match fired";
        softwareInterfaceAdded(asyncResp, m);
    };

    fwUpdateInProgress = true;

    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match::match>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);
}

class UpdateService : public Node
{
  public:
    UpdateService(CrowApp &app) : Node(app, "/redfish/v1/UpdateService/")
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
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue["@odata.type"] = "#UpdateService.v1_2_0.UpdateService";
        res.jsonValue["@odata.id"] = "/redfish/v1/UpdateService";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#UpdateService.UpdateService";
        res.jsonValue["Id"] = "UpdateService";
        res.jsonValue["Description"] = "Service for Software Update";
        res.jsonValue["Name"] = "Update Service";
        res.jsonValue["HttpPushUri"] = "/redfish/v1/UpdateService";
        // UpdateService cannot be disabled
        res.jsonValue["ServiceEnabled"] = true;
        res.jsonValue["FirmwareInventory"] = {
            {"@odata.id", "/redfish/v1/UpdateService/FirmwareInventory"}};
        res.end();
    }

    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "doPost...";

        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);

        // Setup callback for when new software detected
        monitorForSoftwareAvailable(asyncResp, req);

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
        res.jsonValue["@odata.type"] =
            "#SoftwareInventoryCollection.SoftwareInventoryCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/UpdateService/FirmwareInventory";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/"
            "$metadata#SoftwareInventoryCollection.SoftwareInventoryCollection";
        res.jsonValue["Name"] = "Software Inventory Collection";

        crow::connections::systemBus->async_method_call(
            [asyncResp](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;

                for (auto &obj : subtree)
                {
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>
                        &connections = obj.second;

                    // if can't parse fw id then return
                    std::size_t idPos;
                    if ((idPos = obj.first.rfind("/")) == std::string::npos)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_DEBUG << "Can't parse firmware ID!!";
                        return;
                    }
                    std::string swId = obj.first.substr(idPos + 1);

                    for (auto &conn : connections)
                    {
                        const std::string &connectionName = conn.first;
                        BMCWEB_LOG_DEBUG << "connectionName = "
                                         << connectionName;
                        BMCWEB_LOG_DEBUG << "obj.first = " << obj.first;

                        crow::connections::systemBus->async_method_call(
                            [asyncResp,
                             swId](const boost::system::error_code error_code,
                                   const VariantType &activation) {
                                BMCWEB_LOG_DEBUG
                                    << "safe returned in lambda function";
                                if (error_code)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }

                                const std::string *swActivationStatus =
                                    std::get_if<std::string>(&activation);
                                if (swActivationStatus == nullptr)
                                {
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                if (swActivationStatus != nullptr &&
                                    *swActivationStatus !=
                                        "xyz.openbmc_project.Software."
                                        "Activation."
                                        "Activations.Active")
                                {
                                    // The activation status of this software is
                                    // not currently active, so does not need to
                                    // be listed in the response
                                    return;
                                }
                                nlohmann::json &members =
                                    asyncResp->res.jsonValue["Members"];
                                members.push_back(
                                    {{"@odata.id", "/redfish/v1/UpdateService/"
                                                   "FirmwareInventory/" +
                                                       swId}});
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
        entityPrivileges = {
            {boost::beast::http::verb::get, {{"Login"}}},
            {boost::beast::http::verb::head, {{"Login"}}},
            {boost::beast::http::verb::patch, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::put, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::delete_, {{"ConfigureComponents"}}},
            {boost::beast::http::verb::post, {{"ConfigureComponents"}}}};
    }

  private:
    /* Fill related item links (i.e. bmc, bios) in for inventory */
    static void getRelatedItems(std::shared_ptr<AsyncResp> aResp,
                                const std::string &purpose)
    {
        if (purpose == fw_util::bmcPurpose)
        {
            nlohmann::json &members = aResp->res.jsonValue["RelatedItem"];
            members.push_back({{"@odata.id", "/redfish/v1/Managers/bmc"}});
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        }
        else if (purpose == fw_util::biosPurpose)
        {
            // TODO(geissonator) Need BIOS schema support added for this
            //                   to be valid
            // nlohmann::json &members = aResp->res.jsonValue["RelatedItem"];
            // members.push_back(
            //    {{"@odata.id", "/redfish/v1/Systems/system/BIOS"}});
            // aResp->res.jsonValue["Members@odata.count"] = members.size();
        }
        else
        {
            BMCWEB_LOG_ERROR << "Unknown software purpose " << purpose;
        }
    }

    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        res.jsonValue["@odata.type"] =
            "#SoftwareInventory.v1_1_0.SoftwareInventory";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#SoftwareInventory.SoftwareInventory";
        res.jsonValue["Name"] = "Software Inventory";
        res.jsonValue["Updateable"] = false;
        res.jsonValue["Status"]["Health"] = "OK";
        res.jsonValue["Status"]["HealthRollup"] = "OK";
        res.jsonValue["Status"]["State"] = "Enabled";

        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }

        std::shared_ptr<std::string> swId =
            std::make_shared<std::string>(params[0]);

        res.jsonValue["@odata.id"] =
            "/redfish/v1/UpdateService/FirmwareInventory/" + *swId;

        crow::connections::systemBus->async_method_call(
            [asyncResp, swId](
                const boost::system::error_code ec,
                const std::vector<std::pair<
                    std::string, std::vector<std::pair<
                                     std::string, std::vector<std::string>>>>>
                    &subtree) {
                BMCWEB_LOG_DEBUG << "doGet callback...";
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    return;
                }

                for (const std::pair<
                         std::string,
                         std::vector<
                             std::pair<std::string, std::vector<std::string>>>>
                         &obj : subtree)
                {
                    if (boost::ends_with(obj.first, *swId) != true)
                    {
                        continue;
                    }

                    if (obj.second.size() < 1)
                    {
                        continue;
                    }

                    crow::connections::systemBus->async_method_call(
                        [asyncResp,
                         swId](const boost::system::error_code error_code,
                               const boost::container::flat_map<
                                   std::string, VariantType> &propertiesList) {
                            if (error_code)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            boost::container::flat_map<
                                std::string, VariantType>::const_iterator it =
                                propertiesList.find("Purpose");
                            if (it == propertiesList.end())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Can't find property \"Purpose\"!";
                                messages::propertyMissing(asyncResp->res,
                                                          "Purpose");
                                return;
                            }
                            const std::string *swInvPurpose =
                                std::get_if<std::string>(&it->second);
                            if (swInvPurpose == nullptr)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "wrong types for property\"Purpose\"!";
                                messages::propertyValueTypeError(asyncResp->res,
                                                                 "", "Purpose");
                                return;
                            }

                            BMCWEB_LOG_DEBUG << "swInvPurpose = "
                                             << *swInvPurpose;
                            it = propertiesList.find("Version");
                            if (it == propertiesList.end())
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Can't find property \"Version\"!";
                                messages::propertyMissing(asyncResp->res,
                                                          "Version");
                                return;
                            }

                            BMCWEB_LOG_DEBUG << "Version found!";

                            const std::string *version =
                                std::get_if<std::string>(&it->second);

                            if (version == nullptr)
                            {
                                BMCWEB_LOG_DEBUG
                                    << "Can't find property \"Version\"!";

                                messages::propertyValueTypeError(asyncResp->res,
                                                                 "", "Version");
                                return;
                            }
                            asyncResp->res.jsonValue["Version"] = *version;
                            asyncResp->res.jsonValue["Id"] = *swId;

                            // swInvPurpose is of format:
                            // xyz.openbmc_project.Software.Version.VersionPurpose.ABC
                            // Translate this to "ABC update"
                            size_t endDesc = swInvPurpose->rfind(".");
                            if (endDesc == std::string::npos)
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }
                            endDesc++;
                            if (endDesc >= swInvPurpose->size())
                            {
                                messages::internalError(asyncResp->res);
                                return;
                            }

                            std::string formatDesc =
                                swInvPurpose->substr(endDesc);
                            asyncResp->res.jsonValue["Description"] =
                                formatDesc + " update";
                            getRelatedItems(asyncResp, *swInvPurpose);
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
