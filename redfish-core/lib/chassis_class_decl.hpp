#pragma once

#include "app_class_decl.hpp"
using crow::App;

namespace redfish
{

inline void getChassisState(std::shared_ptr<bmcweb::AsyncResp> aResp);
inline void getPhysicalSecurityData(std::shared_ptr<bmcweb::AsyncResp> aResp);
inline void getIntrusionByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                  const std::string& service,
                                  const std::string& objPath);
inline void doChassisPowerCycle(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void requestRoutesChassisCollection(App& app);
void requestRoutesChassis(App& app);
void requestRoutesChassisResetAction(App& app);
void requestRoutesChassisResetActionInfo(App& app);

}