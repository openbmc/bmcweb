/*
// Copyright (c) 2018 Intel Corporation
// Copyright (c) 2018 Ampere Computing LLC
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

using GetManagedObjectsType = boost::container::flat_map<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<
            std::string, sdbusplus::message::variant<
                             std::string, bool, uint8_t, int16_t, uint16_t,
                             int32_t, uint32_t, int64_t, uint64_t, double>>>>;

inline std::string translateSeverityDbusToRedfish(const std::string s)
{
    if (s == "xyz.openbmc_project.Logging.Entry.Level.Alert")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Critical")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Debug")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Emergency")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Error")
    {
        return "Critical";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Information")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Notice")
    {
        return "OK";
    }
    else if (s == "xyz.openbmc_project.Logging.Entry.Level.Warning")
    {
        return "Warning";
    }
    return "";
}

/**
 * LogEntryCollection derived class for delivering Log Entry Collection Schema
 */
class LogEntryCollection : public Node
{
  public:
    template <typename CrowApp>
    LogEntryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/1/LogServices/SEL/Entries/")
    {
        Node::json["@odata.type"] = "#LogEntryCollection.LogEntryCollection";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#LogEntryCollection.LogEntryCollection";
        Node::json["@odata.id"] =
            "/redfish/v1/Systems/1/LogServices/SEL/Entries";
        Node::json["Description"] = "Collection of Logs for this System";
        Node::json["Name"] = "Log Service Collection";

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
     * Functions triggers appropriate requests on D-Bus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        // Create asyncResp pointer to object holding the response data
        auto asyncResp = std::make_shared<AsyncResp>(res);

        res.jsonValue = Node::json;

        // Make call to Logging Service to find all log entry objects
        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec,
                        GetManagedObjectsType &resp) {
                if (ec)
                {
                    // TODO Handle for specific error code
                    BMCWEB_LOG_ERROR
                        << "getLogEntriesIfaceData resp_handler got error "
                        << ec;
                    asyncResp->res.result(
                        boost::beast::http::status::internal_server_error);
                    return;
                }
                nlohmann::json &entriesArray =
                    asyncResp->res.jsonValue["Entries"];
                entriesArray = nlohmann::json::array();
                nlohmann::json thisEntry;
                for (auto &objectPath : resp)
                {
                    for (auto &interfaceMap : objectPath.second)
                    {
                        if (interfaceMap.first !=
                            "xyz.openbmc_project.Logging.Entry")
                        {
                            BMCWEB_LOG_DEBUG << "Bailing early on "
                                             << interfaceMap.first;
                            continue;
                        }

                        for (auto &propertyMap : interfaceMap.second)
                        {
                            thisEntry["@odata.type"] =
                                "#LogService.v1_3_0.LogService";
                            thisEntry["@odata.context"] =
                                "/redfish/v1/$metadata#LogEntry.LogEntry";
                            thisEntry["EntryType"] = "SEL"; // System Event Log
                            if (propertyMap.first == "Id")
                            {
                                const uint32_t *id =
                                    mapbox::getPtr<const uint32_t>(
                                        propertyMap.second);
                                if (id != nullptr)
                                {
                                    thisEntry["Id"] = *id;
                                    thisEntry["Name"] = std::to_string(*id);
                                }
                            }
                            else if (propertyMap.first == "Timestamp")
                            {
                                const uint64_t *millisTimeStamp =
                                    mapbox::getPtr<const uint64_t>(
                                        propertyMap.second);
                                if (millisTimeStamp != nullptr)
                                {
                                    // Retrieve Created property with format:
                                    // yyyy-mm-ddThh:mm:ss
                                    std::chrono::milliseconds chronoTimeStamp(
                                        *millisTimeStamp);
                                    std::time_t timestamp =
                                        std::chrono::duration_cast<
                                            std::chrono::seconds>(
                                            chronoTimeStamp)
                                            .count();
                                    thisEntry["Created"] =
                                        crow::utility::getDateTime(timestamp);
                                }
                            }
                            else if (propertyMap.first == "Severity")
                            {
                                const std::string *severity =
                                    mapbox::getPtr<const std::string>(
                                        propertyMap.second);
                                if (severity != nullptr)
                                {
                                    thisEntry["Severity"] =
                                        translateSeverityDbusToRedfish(
                                            *severity);
                                }
                            }
                            else if (propertyMap.first == "Message")
                            {
                                const std::string *message =
                                    mapbox::getPtr<const std::string>(
                                        propertyMap.second);
                                if (message != nullptr)
                                {
                                    thisEntry["Message"] = *message;
                                }
                            }
                            else if (propertyMap.first == "Resolved")
                            {
                                const bool *resolved =
                                    mapbox::getPtr<const bool>(
                                        propertyMap.second);
                                if (resolved != nullptr)
                                {
                                    // No place to put this for now
                                }
                            }
                            else
                            {
                                BMCWEB_LOG_WARNING << "Got extra property in "
                                                      "log entry interface "
                                                   << propertyMap.first;
                            }
                        }
                        entriesArray.push_back(std::move(thisEntry));
                        // Not sure if this is neccesary, but technically move
                        // could leave thisEntry in any state it wants
                        thisEntry.clear();
                    }
                }
            },
            "xyz.openbmc_project.Logging", "/xyz/openbmc_project/logging",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
    }
};

/**
 * LogServiceActionsClear class supports POST method for ClearLog action.
 */
class LogServiceActionsClear : public Node
{
  public:
    LogServiceActionsClear(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/1/LogServices/SEL/Actions/"
                  "LogService.Reset")
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
     * Function handles POST method request.
     * The Clear Log actions does not require any parameter.The action deletes
     * all entries found in the Entries collection for this Log Service.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        BMCWEB_LOG_DEBUG << "Do delete all entries.";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        // Process response from Logging service.
        auto resp_handler = [asyncResp](const boost::system::error_code ec) {
            BMCWEB_LOG_DEBUG << "doClearLog resp_handler callback: Done";
            if (ec)
            {
                // TODO Handle for specific error code
                BMCWEB_LOG_ERROR << "doClearLog resp_handler got error " << ec;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }

            asyncResp->res.result(boost::beast::http::status::no_content);
        };

        // Make call to Logging service to request Clear Log
        crow::connections::systemBus->async_method_call(
            resp_handler, "xyz.openbmc_project.Logging",
            "/xyz/openbmc_project/logging",
            "xyz.openbmc_project.Collection.DeleteAll", "DeleteAll");
    }
};

/**
 * LogService derived class for delivering Log Service Schema.
 */
class LogService : public Node
{
  public:
    template <typename CrowApp>
    LogService(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/1/LogServices/SEL/")
    {
        Node::json["@odata.type"] = "#LogService.v1_1_0.LogService";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#LogService.LogService";
        Node::json["Name"] = "System Log Service";
        Node::json["Id"] = "SEL"; // System Event Log
        Node::json["Entries"] = {
            {"@odata.id", "/redfish/v1/Systems/1/LogServices/SEL/Entries"}};

        Node::json["ServiceEnabled"] = true;
        // TODO hardcoded Status information
        Node::json["Status"]["State"] = "Enabled";
        Node::json["Status"]["Health"] = "OK";

        Node::json["Actions"]["#LogService.ClearLog"]["target"] =
            "/redfish/v1/Systems/1/LogServices/SEL/Actions/LogService.Reset";

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
     * Functions triggers appropriate requests on D-Bus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue = Node::json;
        // TODO Logging service has not supported get MaxNumberOfRecords
        // property yet hardcode to ERROR_CAP (200) from phosphor-logging.
        res.jsonValue["MaxNumberOfRecords"] = 200;
        // TODO hardcoded should retrieve from Logging service
        res.jsonValue["OverWritePolicy"] = "WrapsWhenFull";
        // Get DateTime with format: yyyy-mm-ddThh:mm:ssZhh:mm
        const std::time_t time = std::time(nullptr);
        std::string redfishDateTime = crow::utility::getDateTime(time);
        redfishDateTime.insert(redfishDateTime.end() - 2, ':'); // hh:mm format
        res.jsonValue["DateTime"] = redfishDateTime;
        // Get DateTimeLocalOffset with format: Zhh:mm
        // Time Zone position
        res.jsonValue["DateTimeLocalOffset"] =
            redfishDateTime.substr(redfishDateTime.length() - 6);
        res.end();
    }
};

/**
 * LogServiceCollection derived class for delivering
 * Log Service Collection Schema
 */
class LogServiceCollection : public Node
{
  public:
    template <typename CrowApp>
    LogServiceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/1/LogServices/")
    {
        Node::json["@odata.type"] =
            "#LogServiceCollection.LogServiceCollection";
        Node::json["@odata.id"] = "/redfish/v1/Systems/1/LogServices";
        Node::json["@odata.context"] =
            "/redfish/v1/$metadata#LogServiceCollection.LogServiceCollection";
        Node::json["Name"] = "Log Services Collection";
        Node::json["Members"] = {
            {{"@odata.id", "/redfish/v1/Systems/1/LogServices/SEL"}}};
        Node::json["Members@odata.count"] = 1;

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
     * Functions triggers appropriate requests on D-Bus
     */
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        res.jsonValue = Node::json;
        res.end();
    }
};
} // namespace redfish
