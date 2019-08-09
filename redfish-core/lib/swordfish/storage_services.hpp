/*
// Copyright (c) 2019 Intel Corporation
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

#include <node.hpp>

namespace redfish
{

// VROC Swordfish API specific types.
// To be moved to separate VROC utils file in the future.
using SsiResponseItem = boost::container::flat_map<
    std::string,
    std::variant<bool, uint8_t, uint16_t, uint32_t, uint64_t, std::string,
        std::vector<std::string>>>;
using SsiResponse = boost::container::flat_map<
    std::string,
    std::variant<bool, uint8_t, uint16_t, uint32_t, std::string,
        std::vector<std::string>, std::vector<SsiResponseItem>>>;

class StorageServiceCollection : public Node
{
  public:
    StorageServiceCollection(CrowApp &app) :
        Node(app, "/redfish/v1/StorageServices")
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
        res.jsonValue = {
            {"@odata.id", "/redfish/v1/StorageServices"},
            {"@odata.type",
                "#StorageServiceCollection.StorageServiceCollection"},
            {"@odata.context", "/redfish/v1/$metadata"
                "#StorageServiceCollection.StorageServiceCollection"},
            {"Name", "Storage Service Collection"}
        };

        nlohmann::json &storageServices = res.jsonValue["Members"];
        storageServices = nlohmann::json::array();
#ifdef BMCWEB_ENABLE_VROC_SWORDFISH
        storageServices.push_back(
            {{"@odata.id", "/redfish/v1/StorageServices/VROC"}});
#endif
        res.jsonValue["Members@odata.count"] = storageServices.size();
        res.end();
    }
};

class StorageService : public Node
{
  public:
    StorageService(CrowApp &app) :
        Node(app, "/redfish/v1/StorageServices/<str>", std::string())
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
    void getVROCStorageService(const std::shared_ptr<AsyncResp>& asyncRes)
    {
        asyncRes->res.jsonValue = {
            {"@odata.id", "/redfish/v1/StorageServices/VROC"},
            {"@odata.type", "#StorageService.v1_2_0.StorageService"},
            {"@odata.context",
                "/redfish/v1/$metadata#StorageService.StorageService"},
            {"Id", "VROC"},
            {"Name", "VROC Storage Service"},
            {"Description", "Virtual RAID on CPU Out-of-band Storage Service"},
            {"Status", {
                {"Health", "OK"},
                {"State", "Enabled"}
            }},
            {"Links", {
                {"HostingSystem", {
                    {"@odata.id", "/redfish/v1/Systems/system"}
                }}
            }},
            {"StorageSubsystems", {
                {"@odata.id", "/redfish/v1/StorageServices/VROC/Storage"}
            }},
            {"StoragePools", {
                {"@odata.id", "/redfish/v1/StorageServices/VROC/StoragePools"}
            }},
            {"Volumes", {
                {"@odata.id", "/redfish/v1/StorageServices/VROC/Volumes"}
            }}
        };

        //Get Oem properties
        crow::connections::systemBus->async_method_call(
            [asyncRes](const boost::system::error_code errorCode,
                        const SsiResponse &executorInfo) {
                if (errorCode)
                {
                    asyncRes->res.jsonValue["Status"]["Health"] = "Critical";
                    asyncRes->res.jsonValue["Status"]["State"] =
                        "UnavailableOffline";
                    messages::internalError(asyncRes->res);
                    return;
                }

                for (const auto &property : executorInfo)
                {
                    if (nullptr != std::get_if<std::string>(&property.second))
                    {
                        asyncRes->res.jsonValue["Oem"][property.first] =
                            property.second;
                    }
                }
            },
            "xyz.openbmc_project.Issm", "/xyz/openbmc_project/Issm",
            "xyz.openbmc_project.issm.vroc", "GetExecutorInformation");
    }

    void doGet(crow::Response &res, const crow::Request &req,
               const std::vector<std::string> &params) override
    {
        const std::string &storageServiceId = params[0];
        auto asyncRes = std::make_shared<AsyncResp>(res); 

#ifdef BMCWEB_ENABLE_VROC_SWORDFISH
        if (storageServiceId == "VROC") 
        {
            getVROCStorageService(asyncRes);
            return;
        }
#endif

        messages::resourceNotFound(res, 
            "#StorageService.v1_2_0.StorageService", storageServiceId);
    }
};

} // namespace redfish