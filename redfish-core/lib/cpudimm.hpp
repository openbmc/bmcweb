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

void getResourceList(std::shared_ptr<AsyncResp> aResp, const std::string &name,
                     const std::string &subclass,
                     const std::string &collectionName)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu/mem resources.";
    crow::connections::systemBus->async_method_call(
        [name, subclass, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>
                &subtree) {
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
                auto iter = object.first.rfind("/");
                if ((iter != std::string::npos) && (iter < object.first.size()))
                {
                    members.push_back(
                        {{"@odata.id", "/redfish/v1/Systems/" + name + "/" +
                                           subclass + "/" +
                                           object.first.substr(iter + 1)}});
                }
            }
            aResp->res.jsonValue["Members@odata.count"] = members.size();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 1>{collectionName.c_str()});
}

void getCpuDataByService(std::shared_ptr<AsyncResp> aResp,
                         const std::string &name, const std::string &cpuId,
                         const std::string &service, const std::string &objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources by service.";
    crow::connections::systemBus->async_method_call(
        [name, cpuId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string,
                sdbusplus::message::variant<std::string, uint32_t, uint16_t>>
                &properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            aResp->res.jsonValue["Id"] = cpuId;
            aResp->res.jsonValue["Name"] = "Processor";
            const auto coresCountProperty =
                properties.find("ProcessorCoreCount");
            if (coresCountProperty == properties.end())
            {
                // Important property not in result
                messages::internalError(aResp->res);
                return;
            }
            const uint16_t *coresCount =
                mapbox::getPtr<const uint16_t>(coresCountProperty->second);
            if (coresCount == nullptr)
            {
                // Important property not in desired type
                messages::internalError(aResp->res);
                return;
            }
            if (*coresCount == 0)
            {
                // Slot is not populated, set status end return
                aResp->res.jsonValue["Status"]["State"] = "Absent";
                aResp->res.jsonValue["Status"]["Health"] = "OK";
                // HTTP Code will be set up automatically, just return
                return;
            }

            aResp->res.jsonValue["TotalCores"] = *coresCount;
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
            aResp->res.jsonValue["Status"]["Health"] = "OK";

            for (const auto &property : properties)
            {
                if (property.first == "ProcessorType")
                {
                    aResp->res.jsonValue["Name"] = property.second;
                }
                else if (property.first == "ProcessorManufacturer")
                {
                    aResp->res.jsonValue["Manufacturer"] = property.second;
                    const std::string *value =
                        mapbox::getPtr<const std::string>(property.second);
                    if (value != nullptr)
                    {
                        // Otherwise would be unexpected.
                        if (value->find("Intel") != std::string::npos)
                        {
                            aResp->res.jsonValue["ProcessorArchitecture"] =
                                "x86";
                            aResp->res.jsonValue["InstructionSet"] = "x86-64";
                        }
                    }
                }
                else if (property.first == "ProcessorMaxSpeed")
                {
                    aResp->res.jsonValue["MaxSpeedMHz"] = property.second;
                }
                else if (property.first == "ProcessorThreadCount")
                {
                    aResp->res.jsonValue["TotalThreads"] = property.second;
                }
                else if (property.first == "ProcessorVersion")
                {
                    aResp->res.jsonValue["Model"] = property.second;
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

void getCpuData(std::shared_ptr<AsyncResp> aResp, const std::string &name,
                const std::string &cpuId)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources.";
    crow::connections::systemBus->async_method_call(
        [name, cpuId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>
                &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto &object : subtree)
            {
                if (boost::ends_with(object.first, cpuId))
                {
                    for (const auto &service : object.second)
                    {
                        getCpuDataByService(aResp, name, cpuId, service.first,
                                            object.first);
                        return;
                    }
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Processor", cpuId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 1>{"xyz.openbmc_project.Inventory.Item.Cpu"});
};

void getDimmDataByService(std::shared_ptr<AsyncResp> aResp,
                          const std::string &name, const std::string &dimmId,
                          const std::string &service,
                          const std::string &objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system components.";
    crow::connections::systemBus->async_method_call(
        [name, dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string,
                sdbusplus::message::variant<std::string, uint32_t, uint16_t>>
                &properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            aResp->res.jsonValue["Id"] = dimmId;
            aResp->res.jsonValue["Name"] = "DIMM Slot";

            const auto memorySizeProperty = properties.find("MemorySizeInKB");
            if (memorySizeProperty == properties.end())
            {
                // Important property not in result
                messages::internalError(aResp->res);

                return;
            }
            const uint32_t *memorySize =
                mapbox::getPtr<const uint32_t>(memorySizeProperty->second);
            if (memorySize == nullptr)
            {
                // Important property not in desired type
                messages::internalError(aResp->res);

                return;
            }
            if (*memorySize == 0)
            {
                // Slot is not populated, set status end return
                aResp->res.jsonValue["Status"]["State"] = "Absent";
                aResp->res.jsonValue["Status"]["Health"] = "OK";
                // HTTP Code will be set up automatically, just return
                return;
            }
            aResp->res.jsonValue["CapacityMiB"] = (*memorySize >> 10);
            aResp->res.jsonValue["Status"]["State"] = "Enabled";
            aResp->res.jsonValue["Status"]["Health"] = "OK";

            for (const auto &property : properties)
            {
                if (property.first == "MemoryDataWidth")
                {
                    aResp->res.jsonValue["DataWidthBits"] = property.second;
                }
                else if (property.first == "MemoryType")
                {
                    const auto *value =
                        mapbox::getPtr<const std::string>(property.second);
                    if (value != nullptr)
                    {
                        aResp->res.jsonValue["MemoryDeviceType"] = *value;
                        if (boost::starts_with(*value, "DDR"))
                        {
                            aResp->res.jsonValue["MemoryType"] = "DRAM";
                        }
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

void getDimmData(std::shared_ptr<AsyncResp> aResp, const std::string &name,
                 const std::string &dimmId)
{
    BMCWEB_LOG_DEBUG << "Get available system dimm resources.";
    crow::connections::systemBus->async_method_call(
        [name, dimmId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>
                &subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);

                return;
            }
            for (const auto &object : subtree)
            {
                if (boost::ends_with(object.first, dimmId))
                {
                    for (const auto &service : object.second)
                    {
                        getDimmDataByService(aResp, name, dimmId, service.first,
                                             object.first);
                        return;
                    }
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Memory", dimmId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char *, 1>{"xyz.openbmc_project.Inventory.Item.Dimm"});
};

class ProcessorCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    ProcessorCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/Processors/", std::string())
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
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);
            res.end();
            return;
        }
        const std::string &name = params[0];

        res.jsonValue"@odata.type"] = "#ProcessorCollection.ProcessorCollection";
        res.jsonValue["Name"] = "Processor Collection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#ProcessorCollection.ProcessorCollection";

        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/" + name + "/Processors/";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getResourceList(asyncResp, name, "Processors",
                        "xyz.openbmc_project.Inventory.Item.Cpu");
    }
};

class Processor : public Node
{
  public:
    /*
     * Default Constructor
     */
    Processor(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/Processors/<str>/", std::string(),
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
        const std::string &cpuId = params[1];
        res.jsonValue["@odata.type"] = "#Processor.v1_1_0.Processor";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#Processor.Processor";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/" + name + "/Processors/" + cpuId;

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getCpuData(asyncResp, name, cpuId);
    }
};

class MemoryCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    MemoryCollection(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/Memory/", std::string())
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
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string &name = params[0];

        res.jsonValue["@odata.type"] = "#MemoryCollection.MemoryCollection";
        res.jsonValue["Name"] = "Memory Module Collection";
        res.jsonValue["@odata.context"] =
            "/redfish/v1/$metadata#MemoryCollection.MemoryCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/" + name + "/Memory/";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getResourceList(asyncResp, name, "Memory",
                        "xyz.openbmc_project.Inventory.Item.Dimm");
    }
};

class Memory : public Node
{
  public:
    /*
     * Default Constructor
     */
    Memory(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/<str>/Memory/<str>/", std::string(),
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
        const std::string &dimmId = params[1];

        res.jsonValue["@odata.type"] = "#Memory.v1_2_0.Memory";
        res.jsonValue["@odata.context"] = "/redfish/v1/$metadata#Memory.Memory";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/" + name + "/Memory/" + dimmId;
        auto asyncResp = std::make_shared<AsyncResp>(res);

        getDimmData(asyncResp, name, dimmId);
    }
};

} // namespace redfish
