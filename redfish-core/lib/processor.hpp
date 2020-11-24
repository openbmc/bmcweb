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

#include "health.hpp"

#include <boost/container/flat_map.hpp>
#include <node.hpp>
#include <utils/collection.hpp>
#include <utils/json_utils.hpp>

namespace redfish
{

using InterfacesProperties = boost::container::flat_map<
    std::string,
    boost::container::flat_map<std::string, dbus::utility::DbusVariantType>>;

inline void
    getCpuDataByInterface(const std::shared_ptr<AsyncResp>& aResp,
                          const InterfacesProperties& cpuInterfacesProperties)
{
    BMCWEB_LOG_DEBUG << "Get CPU resources by interface.";

    // Added for future purpose. Once present and functional attributes added
    // in busctl call, need to add actual logic to fetch original values.
    bool present = false;
    const bool functional = true;
    auto health = std::make_shared<HealthPopulate>(aResp);
    health->populate();

    for (const auto& interface : cpuInterfacesProperties)
    {
        for (const auto& property : interface.second)
        {
            if (property.first == "CoreCount")
            {
                const uint16_t* coresCount =
                    std::get_if<uint16_t>(&property.second);
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
                    // HTTP Code will be set up automatically, just return
                    return;
                }
                aResp->res.jsonValue["Status"]["State"] = "Enabled";
                present = true;
                aResp->res.jsonValue["TotalCores"] = *coresCount;
            }
            else if (property.first == "MaxSpeedInMhz")
            {
                const uint32_t* value = std::get_if<uint32_t>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["MaxSpeedMHz"] = *value;
                }
            }
            else if (property.first == "Socket")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["Socket"] = *value;
                }
            }
            else if (property.first == "ThreadCount")
            {
                const uint16_t* value = std::get_if<uint16_t>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["TotalThreads"] = *value;
                }
            }
            else if (property.first == "Family")
            {
                const std::string* value =
                    std::get_if<std::string>(&property.second);
                if (value != nullptr)
                {
                    aResp->res.jsonValue["ProcessorId"]["EffectiveFamily"] =
                        *value;
                }
            }
            else if (property.first == "Id")
            {
                const uint64_t* value = std::get_if<uint64_t>(&property.second);
                if (value != nullptr && *value != 0)
                {
                    present = true;
                    aResp->res
                        .jsonValue["ProcessorId"]["IdentificationRegisters"] =
                        boost::lexical_cast<std::string>(*value);
                }
            }
        }
    }

    if (present == false)
    {
        aResp->res.jsonValue["Status"]["State"] = "Absent";
        aResp->res.jsonValue["Status"]["Health"] = "OK";
    }
    else
    {
        aResp->res.jsonValue["Status"]["State"] = "Enabled";
        if (functional)
        {
            aResp->res.jsonValue["Status"]["Health"] = "OK";
        }
        else
        {
            aResp->res.jsonValue["Status"]["Health"] = "Critical";
        }
    }

    return;
}

inline void getCpuDataByService(std::shared_ptr<AsyncResp> aResp,
                                const std::string& cpuId,
                                const std::string& service,
                                const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu resources by service.";

    crow::connections::systemBus->async_method_call(
        [cpuId, service, objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::ManagedObjectType& dbusData) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["Id"] = cpuId;
            aResp->res.jsonValue["Name"] = "Processor";
            aResp->res.jsonValue["ProcessorType"] = "CPU";

            bool slotPresent = false;
            std::string corePath = objPath + "/core";
            size_t totalCores = 0;
            for (const auto& object : dbusData)
            {
                if (object.first.str == objPath)
                {
                    getCpuDataByInterface(aResp, object.second);
                }
                else if (boost::starts_with(object.first.str, corePath))
                {
                    std::size_t index = object.first.str.rfind('/');
                    if (index == std::string::npos)
                    {
                        return;
                    }

                    for (const auto& interface : object.second)
                    {
                        if (interface.first ==
                            "xyz.openbmc_project.Inventory.Item")
                        {
                            for (const auto& property : interface.second)
                            {
                                if (property.first == "Present")
                                {
                                    const bool* present =
                                        std::get_if<bool>(&property.second);
                                    if (present != nullptr)
                                    {
                                        if (*present == true)
                                        {
                                            slotPresent = true;
                                            totalCores++;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // In getCpuDataByInterface(), state and health are set
            // based on the present and functional status. If core
            // count is zero, then it has a higher precedence.
            if (slotPresent)
            {
                if (totalCores == 0)
                {
                    // Slot is not populated, set status end return
                    aResp->res.jsonValue["Status"]["State"] = "Absent";
                    aResp->res.jsonValue["Status"]["Health"] = "OK";
                }
                aResp->res.jsonValue["TotalCores"] = totalCores;
            }
            return;
        },
        service, "/xyz/openbmc_project/inventory",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void getCpuAssetData(std::shared_ptr<AsyncResp> aResp,
                            const std::string& service,
                            const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Asset Data";
    crow::connections::systemBus->async_method_call(
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                if (property.first == "SerialNumber")
                {
                    const std::string* sn =
                        std::get_if<std::string>(&property.second);
                    if (sn != nullptr && !sn->empty())
                    {
                        aResp->res.jsonValue["SerialNumber"] = *sn;
                    }
                }
                else if (property.first == "Model")
                {
                    const std::string* model =
                        std::get_if<std::string>(&property.second);
                    if (model != nullptr && !model->empty())
                    {
                        aResp->res.jsonValue["Model"] = *model;
                    }
                }
                else if (property.first == "Manufacturer")
                {

                    const std::string* mfg =
                        std::get_if<std::string>(&property.second);
                    if (mfg != nullptr)
                    {
                        aResp->res.jsonValue["Manufacturer"] = *mfg;

                        // Otherwise would be unexpected.
                        if (mfg->find("Intel") != std::string::npos)
                        {
                            aResp->res.jsonValue["ProcessorArchitecture"] =
                                "x86";
                            aResp->res.jsonValue["InstructionSet"] = "x86-64";
                        }
                        else if (mfg->find("IBM") != std::string::npos)
                        {
                            aResp->res.jsonValue["ProcessorArchitecture"] =
                                "Power";
                            aResp->res.jsonValue["InstructionSet"] = "PowerISA";
                        }
                    }
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Asset");
}

inline void getCpuRevisionData(std::shared_ptr<AsyncResp> aResp,
                               const std::string& service,
                               const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get Cpu Revision Data";
    crow::connections::systemBus->async_method_call(
        [objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& property : properties)
            {
                if (property.first == "Version")
                {
                    const std::string* ver =
                        std::get_if<std::string>(&property.second);
                    if (ver != nullptr)
                    {
                        aResp->res.jsonValue["Version"] = *ver;
                    }
                    break;
                }
            }
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.Inventory.Decorator.Revision");
}

inline void getAcceleratorDataByService(std::shared_ptr<AsyncResp> aResp,
                                        const std::string& acclrtrId,
                                        const std::string& service,
                                        const std::string& objPath)
{
    BMCWEB_LOG_DEBUG
        << "Get available system Accelerator resources by service.";
    crow::connections::systemBus->async_method_call(
        [acclrtrId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, std::variant<std::string, uint32_t, uint16_t,
                                          bool>>& properties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            aResp->res.jsonValue["Id"] = acclrtrId;
            aResp->res.jsonValue["Name"] = "Processor";
            const bool* accPresent = nullptr;
            const bool* accFunctional = nullptr;

            for (const auto& property : properties)
            {
                if (property.first == "Functional")
                {
                    accFunctional = std::get_if<bool>(&property.second);
                }
                else if (property.first == "Present")
                {
                    accPresent = std::get_if<bool>(&property.second);
                }
            }

            std::string state = "Enabled";
            std::string health = "OK";

            if (accPresent != nullptr && *accPresent == false)
            {
                state = "Absent";
            }

            if ((accFunctional != nullptr) && (*accFunctional == false))
            {
                if (state == "Enabled")
                {
                    health = "Critical";
                }
            }

            aResp->res.jsonValue["Status"]["State"] = state;
            aResp->res.jsonValue["Status"]["Health"] = health;
            aResp->res.jsonValue["ProcessorType"] = "Accelerator";
        },
        service, objPath, "org.freedesktop.DBus.Properties", "GetAll", "");
}

inline void getProcessorData(std::shared_ptr<AsyncResp> aResp,
                             const std::string& processorId,
                             const std::vector<const char*>& inventoryItems)
{
    BMCWEB_LOG_DEBUG << "Get available system processor resources.";

    crow::connections::systemBus->async_method_call(
        [processorId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& object : subtree)
            {
                if (boost::ends_with(object.first, processorId))
                {
                    for (const auto& service : object.second)
                    {
                        for (const auto& inventory : service.second)
                        {
                            if (inventory == "xyz.openbmc_project."
                                             "Inventory.Decorator.Asset")
                            {
                                getCpuAssetData(aResp, service.first,
                                                object.first);
                            }
                            else if (inventory ==
                                     "xyz.openbmc_project."
                                     "Inventory.Decorator.Revision")
                            {
                                getCpuRevisionData(aResp, service.first,
                                                   object.first);
                            }
                            else if (inventory == "xyz.openbmc_project."
                                                  "Inventory.Item.Cpu")
                            {
                                getCpuDataByService(aResp, processorId,
                                                    service.first,
                                                    object.first);
                            }
                            else if (inventory == "xyz.openbmc_project."
                                                  "Inventory.Item.Accelerator")
                            {
                                getAcceleratorDataByService(aResp, processorId,
                                                            service.first,
                                                            object.first);
                            }
                        }
                    }
                    return;
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "Processor", processorId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, inventoryItems);
}

inline void getCpuCoreDataByService(std::shared_ptr<AsyncResp> aResp,
                                    const std::string& service,
                                    const std::string& objPath)
{
    BMCWEB_LOG_DEBUG << "Get available system cpu core resources by service.";

    crow::connections::systemBus->async_method_call(
        [service, objPath, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const dbus::utility::ManagedObjectType& dbusData) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& object : dbusData)
            {
                if (object.first.str == objPath)
                {
                    for (const auto& interface : object.second)
                    {
                        if (interface.first ==
                            "xyz.openbmc_project.Association.Definitions")
                        {
                            for (const auto& property : interface.second)
                            {
                                if (property.first == "Associations")
                                {
                                    const std::vector<std::tuple<
                                        std::string, std::string, std::string>>*
                                        associations = std::get_if<std::vector<
                                            std::tuple<std::string, std::string,
                                                       std::string>>>(
                                            &property.second);
                                    if (associations != nullptr)
                                    {
                                        aResp->res.jsonValue["Associations"] =
                                            *associations;
                                    }
                                }
                            }
                        }

                        if (interface.first == "xyz.openbmc_project.State."
                                               "Decorator.OperationalStatus")
                        {
                            for (const auto& property : interface.second)
                            {
                                if (property.first == "Functional")
                                {
                                    const bool* functional =
                                        std::get_if<bool>(&property.second);
                                    if (functional != nullptr)
                                    {
                                        aResp->res.jsonValue["Functional"] =
                                            *functional;
                                    }
                                }
                            }
                        }

                        if (interface.first ==
                            "xyz.openbmc_project.Inventory.Item")
                        {
                            for (const auto& property : interface.second)
                            {
                                if (property.first == "Present")
                                {
                                    const bool* present =
                                        std::get_if<bool>(&property.second);
                                    if (present != nullptr)
                                    {
                                        aResp->res.jsonValue["Present"] =
                                            *present;
                                    }
                                }
                                if (property.first == "PrettyName")
                                {
                                    const std::string* prettyName =
                                        std::get_if<std::string>(
                                            &property.second);
                                    if (prettyName != nullptr)
                                    {
                                        aResp->res.jsonValue["PrettyName"] =
                                            *prettyName;
                                    }
                                }
                            }
                        }
                    }
                }
            }

            return;
        },
        service, "/xyz/openbmc_project/inventory",
        "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
}

inline void getSubProcessorData(std::shared_ptr<AsyncResp> aResp,
                                const std::string& processorId,
                                const std::string& coreId,
                                const std::vector<const char*>& inventoryItems)
{
    BMCWEB_LOG_DEBUG << "Get available system sub processor resources.";

    crow::connections::systemBus->async_method_call(
        [processorId, coreId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }
            for (const auto& object : subtree)
            {
                if (boost::ends_with(object.first, processorId))
                {
                    std::string corePath = object.first + "/" + coreId;

                    for (const auto& service : object.second)
                    {
                        getCpuCoreDataByService(aResp, service.first, corePath);
                    }
                    return;
                }
            }
            // Object not found
            messages::resourceNotFound(aResp->res, "SubProcessor", coreId);
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, inventoryItems);
}

inline void
    getSubProcessorMembers(std::shared_ptr<AsyncResp> aResp,
                           const std::string& processorId,
                           const std::vector<const char*>& inventoryItems)
{
    crow::connections::systemBus->async_method_call(
        [processorId, aResp{std::move(aResp)}](
            const boost::system::error_code ec,
            const boost::container::flat_map<
                std::string, boost::container::flat_map<
                                 std::string, std::vector<std::string>>>&
                subtree) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBUS response error";
                messages::internalError(aResp->res);
                return;
            }

            for (const auto& object : subtree)
            {
                if (!boost::ends_with(object.first, processorId))
                {
                    continue;
                }

                for (const auto& service : object.second)
                {
                    crow::connections::systemBus->async_method_call(
                        [processorId, objPath{object.first},
                         aResp{std::move(aResp)}](
                            const boost::system::error_code ec,
                            const dbus::utility::ManagedObjectType& dbusData) {
                            if (ec)
                            {
                                BMCWEB_LOG_DEBUG << "DBUS response error";
                                messages::internalError(aResp->res);
                                return;
                            }

                            nlohmann::json& members =
                                aResp->res.jsonValue["Members"];
                            members = nlohmann::json::array();
                            std::string subProcessorsPath =
                                "/redfish/v1/Systems/system/Processors/" +
                                processorId + "/SubProcessors/";

                            std::string corePath = objPath + "/core";
                            for (const auto& object : dbusData)
                            {
                                if (!boost::starts_with(object.first.str,
                                                        corePath))
                                {
                                    continue;
                                }
                                std::size_t index = object.first.str.rfind('/');
                                if (index == std::string::npos)
                                {
                                    return;
                                }
                                members.push_back(
                                    {{"@odata.id",
                                      subProcessorsPath +
                                          object.first.str.substr(index + 1)}});
                            }
                            aResp->res.jsonValue["Members@odata.count"] =
                                members.size();

                            return;
                        },
                        service.first, "/xyz/openbmc_project/inventory",
                        "org.freedesktop.DBus.ObjectManager",
                        "GetManagedObjects");
                }
                return;
            }
            return;
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, inventoryItems);
}

class ProcessorCollection : public Node
{
  public:
    /*
     * Default Constructor
     */
    ProcessorCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] =
            "#ProcessorCollection.ProcessorCollection";
        res.jsonValue["Name"] = "Processor Collection";

        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Processors";
        auto asyncResp = std::make_shared<AsyncResp>(res);

        collection_util::getCollectionMembers(
            asyncResp, "/redfish/v1/Systems/system/Processors",
            {"xyz.openbmc_project.Inventory.Item.Cpu",
             "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

class Processor : public Node
{
  public:
    /*
     * Default Constructor
     */
    Processor(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/<str>/", std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& processorId = params[0];
        res.jsonValue["@odata.type"] = "#Processor.v1_10_0.Processor";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Processors/" + processorId;
        res.jsonValue["SubProcessors"] = {
            {"@odata.id", "/redfish/v1/Systems/system/Processors/" +
                              processorId + "/SubProcessors"}};

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getProcessorData(asyncResp, processorId,
                         {"xyz.openbmc_project.Inventory.Item.Cpu",
                          "xyz.openbmc_project.Inventory.Decorator.Asset",
                          "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

class SubProcessors : public Node
{
  public:
    /*
     * Default Constructor
     */
    SubProcessors(App& app) :
        Node(app, "/redfish/v1/Systems/system/Processors/<str>/SubProcessors",
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 1)
        {
            messages::internalError(res);

            res.end();
            return;
        }

        const std::string& processorId = params[0];

        res.jsonValue["@odata.type"] =
            "#ProcessorCollection.ProcessorCollection";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Processors/" +
                                     processorId + "/SubProcessors";
        res.jsonValue["Name"] = "Processor Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);
        getSubProcessorMembers(
            asyncResp, processorId,
            {"xyz.openbmc_project.Inventory.Item.Cpu",
             "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

class SubProcessorsCore : public Node
{
  public:
    /*
     * Default Constructor
     */
    SubProcessorsCore(App& app) :
        Node(app,
             "/redfish/v1/Systems/system/Processors/<str>/SubProcessors/<str>",
             std::string(), std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        // Check if there is required param, truly entering this shall be
        // impossible
        if (params.size() != 2)
        {
            messages::internalError(res);

            res.end();
            return;
        }
        const std::string& processorId = params[0];
        const std::string& coreId = params[1];
        res.jsonValue["@odata.type"] = "#SubProcessors.v1_3_0.SubProcessors";
        res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Processors/" +
                                     processorId + "/SubProcessors/" + coreId;
        res.jsonValue["Name"] = "Processor Collection";

        auto asyncResp = std::make_shared<AsyncResp>(res);

        getSubProcessorData(asyncResp, processorId, coreId,
                            {"xyz.openbmc_project.Inventory.Item.Cpu",
                             "xyz.openbmc_project.Inventory.Item.Accelerator"});
    }
};

} // namespace redfish
