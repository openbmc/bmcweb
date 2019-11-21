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

namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
class BiosService : public Node
{
  public:
    BiosService(CrowApp &app) : Node(app, "/redfish/v1/Systems/system/Bios/")
    {
        entityPrivileges = {{boost::beast::http::verb::get, {{"Login"}}}};
    }

  private:
    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Systems/system/Bios";
        asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
        asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
        asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
        asyncResp->res.jsonValue["Id"] = "BIOS";
        asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
            {"target",
             "/redfish/v1/Systems/437XR1138R2/Bios/Actions/Bios.ResetBios"}};
    }
};
/**
 * BiosReset class supports handle POST method for Reset bios.
 * The class retrieves and sends data directly to D-Bus.
 */
class BiosReset : public Node
{
  public:
    BiosReset(CrowApp &app) :
        Node(app, "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios/")
    {
        entityPrivileges = {
            {boost::beast::http::verb::post, {{"ConfigureManager"}}}};
    }

  private:
    /**
     * Function handles POST method request.
     * Analyzes POST body message before sends Reset request data to D-Bus.
     */
    void doPost(crow::Response &res, const crow::Request &req,
                const std::vector<std::string> &params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);

        crow::connections::systemBus->async_method_call(
            [asyncResp](const boost::system::error_code ec) {
                if (ec)
                {
                    BMCWEB_LOG_ERROR << "Failed to reset bios: " << ec;
                    messages::internalError(asyncResp->res);
                    return;
                }
            },
            "org.open_power.Software.Host.Updater",
            "/xyz/openbmc_project/software",
            "xyz.openbmc_project.Common.FactoryReset", "Reset");
    }
};
} // namespace redfish