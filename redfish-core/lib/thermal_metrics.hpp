#pragma once

#include "node.hpp"
#include "sensors.hpp"

namespace redfish
{

class ThermalMetrics : public Node
{
  public:
    ThermalMetrics(App& app) :
        Node((app),
             "/redfish/v1/Chassis/<str>/ThermalSubsystem/ThermalMetrics/",
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }
        const std::string& chassisName = params[0];
        auto sensorAsyncResp = std::make_shared<SensorsAsyncResp>(
            asyncResp, chassisName,
            sensors::dbus::paths.at(sensors::node::thermal),
            sensors::node::thermal);

        // TODO Need to get Chassis Redundancy information.
        getThermalData(sensorAsyncResp);
    }
};

} // namespace redfish