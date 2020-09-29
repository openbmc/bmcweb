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

#include <boost/system/linux_error.hpp>

namespace redfish
{

static constexpr char const* pcieService = "xyz.openbmc_project.PCIe";
static constexpr char const* pciePath = "/xyz/openbmc_project/PCIe";
static constexpr char const* pcieDeviceInterface =
    "xyz.openbmc_project.PCIe.Device";

static inline void
    getPCIeDeviceList(const std::shared_ptr<AsyncResp>& asyncResp,
                      const std::string& name)
{
    auto getPCIeMapCallback = [asyncResp, name](
                                  const boost::system::error_code ec,
                                  std::vector<std::string>& pcieDevicePaths) {
        if (ec)
        {
            BMCWEB_LOG_DEBUG << "no PCIe device paths found ec: "
                             << ec.message();
            // Not an error, system just doesn't have PCIe info
            return;
        }
        nlohmann::json& pcieDeviceList = asyncResp->res.jsonValue[name];
        pcieDeviceList = nlohmann::json::array();
        for (const std::string& pcieDevicePath : pcieDevicePaths)
        {
            size_t devStart = pcieDevicePath.rfind('/');
            if (devStart == std::string::npos)
            {
                continue;
            }

            std::string devName = pcieDevicePath.substr(devStart + 1);
            if (devName.empty())
            {
                continue;
            }
            pcieDeviceList.push_back(
                {{"@odata.id",
                  "/redfish/v1/Systems/system/PCIeDevices/" + devName}});
        }
        asyncResp->res.jsonValue[name + "@odata.count"] = pcieDeviceList.size();
    };
    crow::connections::systemBus->async_method_call(
        std::move(getPCIeMapCallback), "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        std::string(pciePath) + "/", 1, std::array<std::string, 0>());
}

class SystemPCIeDeviceCollection : public Node
{
  public:
    SystemPCIeDeviceCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/PCIeDevices/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        asyncResp->res.jsonValue = {
            {"@odata.type", "#PCIeDeviceCollection.PCIeDeviceCollection"},
            {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices"},
            {"Name", "PCIe Device Collection"},
            {"Description", "Collection of PCIe Devices"},
            {"Members", nlohmann::json::array()},
            {"Members@odata.count", 0}};
        getPCIeDeviceList(asyncResp, "Members");
    }
};

class SystemPCIeDevice : public Node
{
  public:
    SystemPCIeDevice(App& app) :
        Node(app, "/redfish/v1/Systems/system/PCIeDevices/<str>/",
             std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& device = params[0];

        auto getPCIeDeviceCallback =
            [asyncResp,
             device](const boost::system::error_code ec,
                     boost::container::flat_map<std::string,
                                                std::variant<std::string>>&
                         pcieDevProperties) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "failed to get PCIe Device properties ec: "
                        << ec.value() << ": " << ec.message();
                    if (ec.value() ==
                        boost::system::linux_error::bad_request_descriptor)
                    {
                        messages::resourceNotFound(asyncResp->res, "PCIeDevice",
                                                   device);
                    }
                    else
                    {
                        messages::internalError(asyncResp->res);
                    }
                    return;
                }

                asyncResp->res.jsonValue = {
                    {"@odata.type", "#PCIeDevice.v1_4_0.PCIeDevice"},
                    {"@odata.id",
                     "/redfish/v1/Systems/system/PCIeDevices/" + device},
                    {"Name", "PCIe Device"},
                    {"Id", device}};

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["Manufacturer"]);
                    property)
                {
                    asyncResp->res.jsonValue["Manufacturer"] = *property;
                }

                if (std::string* property = std::get_if<std::string>(
                        &pcieDevProperties["DeviceType"]);
                    property)
                {
                    asyncResp->res.jsonValue["DeviceType"] = *property;
                }

                asyncResp->res.jsonValue["PCIeFunctions"] = {
                    {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                      device + "/PCIeFunctions"}};
            };
        std::string escapedPath = std::string(pciePath) + "/" + device;
        dbus::utility::escapePathForDbus(escapedPath);
        crow::connections::systemBus->async_method_call(
            std::move(getPCIeDeviceCallback), pcieService, escapedPath,
            "org.freedesktop.DBus.Properties", "GetAll", pcieDeviceInterface);
    }
};

class SystemPCIeFunctionCollection : public Node
{
  public:
    SystemPCIeFunctionCollection(App& app) :
        Node(app, "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/",
             std::string())
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& device = params[0];
        asyncResp->res.jsonValue = {
            {"@odata.type", "#PCIeFunctionCollection.PCIeFunctionCollection"},
            {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" + device +
                              "/PCIeFunctions"},
            {"Name", "PCIe Function Collection"},
            {"Description",
             "Collection of PCIe Functions for PCIe Device " + device}};

        auto getPCIeDeviceCallback =
            [asyncResp,
             device](const boost::system::error_code ec,
                     boost::container::flat_map<std::string,
                                                std::variant<std::string>>&
                         pcieDevProperties) {
                if (ec)
                {
                    BMCWEB_LOG_DEBUG
                        << "failed to get PCIe Device properties ec: "
                        << ec.value() << ": " << ec.message();
                    if (ec.value() ==
                        boost::system::linux_error::bad_request_descriptor)
                    {
                        messages::resourceNotFound(asyncResp->res, "PCIeDevice",
                                                   device);
                    }
                    else
                    {
                        messages::internalError(asyncResp->res);
                    }
                    return;
                }

                nlohmann::json& pcieFunctionList =
                    asyncResp->res.jsonValue["Members"];
                pcieFunctionList = nlohmann::json::array();
                static constexpr const int maxPciFunctionNum = 8;
                for (int functionNum = 0; functionNum < maxPciFunctionNum;
                     functionNum++)
                {
                    // Check if this function exists by looking for a device ID
                    std::string devIDProperty =
                        "Function" + std::to_string(functionNum) + "DeviceId";
                    std::string* property = std::get_if<std::string>(
                        &pcieDevProperties[devIDProperty]);
                    if (property && !property->empty())
                    {
                        pcieFunctionList.push_back(
                            {{"@odata.id",
                              "/redfish/v1/Systems/system/PCIeDevices/" +
                                  device + "/PCIeFunctions/" +
                                  std::to_string(functionNum)}});
                    }
                }
                asyncResp->res.jsonValue["PCIeFunctions@odata.count"] =
                    pcieFunctionList.size();
            };
        std::string escapedPath = std::string(pciePath) + "/" + device;
        dbus::utility::escapePathForDbus(escapedPath);
        crow::connections::systemBus->async_method_call(
            std::move(getPCIeDeviceCallback), pcieService, escapedPath,
            "org.freedesktop.DBus.Properties", "GetAll", pcieDeviceInterface);
    }
};

class SystemPCIeFunction : public Node
{
  public:
    SystemPCIeFunction(App& app) :
        Node(
            app,
            "/redfish/v1/Systems/system/PCIeDevices/<str>/PCIeFunctions/<str>/",
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        std::shared_ptr<AsyncResp> asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 2)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& device = params[0];
        const std::string& function = params[1];

        auto getPCIeDeviceCallback = [asyncResp, device, function](
                                         const boost::system::error_code ec,
                                         boost::container::flat_map<
                                             std::string,
                                             std::variant<std::string>>&
                                             pcieDevProperties) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG
                    << "failed to get PCIe Device properties ec: " << ec.value()
                    << ": " << ec.message();
                if (ec.value() ==
                    boost::system::linux_error::bad_request_descriptor)
                {
                    messages::resourceNotFound(asyncResp->res, "PCIeDevice",
                                               device);
                }
                else
                {
                    messages::internalError(asyncResp->res);
                }
                return;
            }

            // Check if this function exists by looking for a device ID
            std::string devIDProperty = "Function" + function + "DeviceId";
            if (std::string* property =
                    std::get_if<std::string>(&pcieDevProperties[devIDProperty]);
                property && property->empty())
            {
                messages::resourceNotFound(asyncResp->res, "PCIeFunction",
                                           function);
                return;
            }

            asyncResp->res.jsonValue = {
                {"@odata.type", "#PCIeFunction.v1_2_0.PCIeFunction"},
                {"@odata.id", "/redfish/v1/Systems/system/PCIeDevices/" +
                                  device + "/PCIeFunctions/" + function},
                {"Name", "PCIe Function"},
                {"Id", function},
                {"FunctionId", std::stoi(function)},
                {"Links",
                 {{"PCIeDevice",
                   {{"@odata.id",
                     "/redfish/v1/Systems/system/PCIeDevices/" + device}}}}}};

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "DeviceId"]);
                property)
            {
                asyncResp->res.jsonValue["DeviceId"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "VendorId"]);
                property)
            {
                asyncResp->res.jsonValue["VendorId"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "FunctionType"]);
                property)
            {
                asyncResp->res.jsonValue["FunctionType"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "DeviceClass"]);
                property)
            {
                asyncResp->res.jsonValue["DeviceClass"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "ClassCode"]);
                property)
            {
                asyncResp->res.jsonValue["ClassCode"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "RevisionId"]);
                property)
            {
                asyncResp->res.jsonValue["RevisionId"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function + "SubsystemId"]);
                property)
            {
                asyncResp->res.jsonValue["SubsystemId"] = *property;
            }

            if (std::string* property = std::get_if<std::string>(
                    &pcieDevProperties["Function" + function +
                                       "SubsystemVendorId"]);
                property)
            {
                asyncResp->res.jsonValue["SubsystemVendorId"] = *property;
            }
        };
        std::string escapedPath = std::string(pciePath) + "/" + device;
        dbus::utility::escapePathForDbus(escapedPath);
        crow::connections::systemBus->async_method_call(
            std::move(getPCIeDeviceCallback), pcieService, escapedPath,
            "org.freedesktop.DBus.Properties", "GetAll", pcieDeviceInterface);
    }
};

} // namespace redfish
