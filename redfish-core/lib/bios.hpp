#pragma once

#include <registries/privilege_registry.hpp>
#include <utils/fw_utils.hpp>
#include "app_class_decl.hpp"
using crow::App;

namespace redfish
{
/**
 * BiosService class supports handle get method for bios.
 */
inline void
    handleBiosServiceGet(const crow::Request&,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/Systems/system/Bios";
    asyncResp->res.jsonValue["@odata.type"] = "#Bios.v1_1_0.Bios";
    asyncResp->res.jsonValue["Name"] = "BIOS Configuration";
    asyncResp->res.jsonValue["Description"] = "BIOS Configuration Service";
    asyncResp->res.jsonValue["Id"] = "BIOS";
    asyncResp->res.jsonValue["Actions"]["#Bios.ResetBios"] = {
        {"target", "/redfish/v1/Systems/system/Bios/Actions/Bios.ResetBios"}};

    // Get the ActiveSoftwareImage and SoftwareImages
    fw_util::populateFirmwareInformation(asyncResp, fw_util::biosPurpose, "",
                                         true);
}

void requestRoutesBiosService(crow::App& app);
void requestRoutesBiosReset(crow::App& app);


} // namespace redfish
