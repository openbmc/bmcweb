#pragma once

#include <string_view>
#include <string>
#include <functional>
#include <string>
#include <memory>
#include <vector>
#include <optional>

#include <nlohmann/json.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/string_body.hpp>
#include <boost/beast/http/message.hpp>
#include <boost/container/flat_map.hpp>
#include "../../include/async_resp_class_definition.hpp"

namespace redfish
{

namespace sensors
{

namespace node
{
static constexpr std::string_view power = "Power";
static constexpr std::string_view sensors = "Sensors";
static constexpr std::string_view thermal = "Thermal";
}

}  // namespace sensors

class SensorsAsyncResp
{
public:
    using DataCompleteCb = std::function<void(
        const boost::beast::http::status status,
        const boost::container::flat_map<std::string, std::string>& uriToDbus)>;
    
    struct SensorData
    {
        const std::string name;
        std::string uri;
        const std::string valueKey;
        const std::string dbusPath;
    };
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisIdIn,
                     const std::vector<const char*>& typesIn,
                     const std::string_view& subNode);
    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& chassisIdIn,
                     const std::vector<const char*>& typesIn,
                     const std::string_view& subNode,
                     DataCompleteCb&& creationComplete);
    ~SensorsAsyncResp();
    void addMetadata(const nlohmann::json& sensorObject,
                     const std::string& valueKey, const std::string& dbusPath);
    void updateUri(const std::string& name, const std::string& uri);
    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;
    const std::string chassisId;
    const std::vector<const char*> types;
    const std::string chassisSubNode;

  private:
    std::optional<std::vector<SensorData>> metadata;
    DataCompleteCb dataComplete;
};

}