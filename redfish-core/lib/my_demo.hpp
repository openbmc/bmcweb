#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/action_info.hpp"
#include "generated/enums/chassis.hpp"
#include "generated/enums/resource.hpp"
#include "http_request.hpp"
#include "led.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utils/chassis_utils.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"

#include <asm-generic/errno.h>

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <iostream>


namespace redfish
{

uint8_t getValue()
{
    // 通过DBus获取属性
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(
        "xyz.openbmc_project.CustomManager", "/xyz/openbmc_project/custommanager",
        "org.freedesktop.DBus.Properties", "Get");
    method.append("xyz.openbmc_project.CustomManager.value", "MyValue");
    std::variant<uint8_t> value;
    try
    {
        auto reply = bus.call(method);
        reply.read(value);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "getValue: can't get my value! ERR=" << e.what() << "\n";
        return 0;
    }
    return std::get<uint8_t>(value);
}

bool setValue(uint8_t newValue)
{
    // 通过DBus设置属性
    auto bus = sdbusplus::bus::new_default();
    auto method = bus.new_method_call(
        "xyz.openbmc_project.CustomManager", "/xyz/openbmc_project/custommanager",
        "org.freedesktop.DBus.Properties", "Set");
    method.append("xyz.openbmc_project.CustomManager.value", "MyValue",
                  std::variant<uint8_t>(newValue));
    try 
    {
        bus.call(method);
    }
    catch (const sdbusplus::exception_t& e)
    {
        std::cerr << "setValue: can't set my value! ERR=" << e.what() << "\n";
        return false;
    }
    return true;
}
    
inline void handleMyDemoGet(
    App& app [[maybe_unused]], const crow::Request& req [[maybe_unused]],
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    nlohmann::json& json = asyncResp->res.jsonValue;
    json["@odata.id"] = "/redfish/v1/MyDemo";
    json["@odata.type"] = "#MyDemo.v1_15_0.MyDemo";
    json["Id"] = "MyDemo";
    json["Name"] = "MyDemo";
    json["Description"] = "MyDemo";
    json["Value"] = getValue();
    json["Type"] = "uint8_t";
    
}

inline void handleMyDemoPatch(
    App& app [[maybe_unused]], const crow::Request& req [[maybe_unused]],
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::optional<uint8_t> new_value;
    if (!json_util::readJsonPatch(                                        
            req, asyncResp->res, 
            "Value", new_value 
            ))
    {
        return;
    }
    if (new_value)
    {
        if(setValue(*new_value))
        {
            asyncResp->res.jsonValue["Value"] = *new_value;
        }
        else
        {
            messages::internalError(asyncResp->res);
            return;
        }
    }
}

inline void requestRoutesMyDemo(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/MyDemo/")
        .privileges(redfish::privileges::getMyDemo) 
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleMyDemoGet, std::ref(app)));

    BMCWEB_ROUTE(app, "/redfish/v1/MyDemo/")
        .privileges(redfish::privileges::patchMyDemo) 
        .methods(boost::beast::http::verb::patch)(
            std::bind_front(handleMyDemoPatch, std::ref(app)));

}

} // namespace redfish
