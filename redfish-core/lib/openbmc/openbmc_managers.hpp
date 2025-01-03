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

#include "bmcweb_config.h"

#include <app.hpp>

#include <string>

namespace redfish
{

inline void
    getHandleOemOpenBmc(const crow::Request& /*req*/,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& /*managerId*/)
{
    // Default OEM data
    nlohmann::json& oemOpenbmc = asyncResp->res.jsonValue;
    oemOpenbmc["@odata.type"] = "#OpenBMCManager.v1_0_0.Manager";
    oemOpenbmc["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}#/Oem/OpenBmc",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);

    nlohmann::json::object_t certificates;
    certificates["@odata.id"] =
        boost::urls::format("/redfish/v1/Managers/{}/Truststore/Certificates",
                            BMCWEB_REDFISH_MANAGER_URI_NAME);
    oemOpenbmc["Certificates"] = std::move(certificates);
}

inline void requestRoutesOpenBmcManager(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/<str>#Oem/OpenBmc")
        .privileges(redfish::privileges::getManager)
        .methods(boost::beast::http::verb::get)(getHandleOemOpenBmc);
}

} // namespace redfish
