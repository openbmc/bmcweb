#pragma once

#include "http_response.hpp"

#include <boost/container/flat_map.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

namespace sensors
{
namespace node
{
static constexpr std::string_view power = "Power";
static constexpr std::string_view sensors = "Sensors";
static constexpr std::string_view thermal = "Thermal";
} // namespace node

namespace dbus
{

static const boost::container::flat_map<std::string_view,
                                        std::vector<const char*>>
    paths = {{node::power,
              {"/xyz/openbmc_project/sensors/voltage",
               "/xyz/openbmc_project/sensors/power"}},
             {node::sensors,
              {"/xyz/openbmc_project/sensors/power",
               "/xyz/openbmc_project/sensors/current",
               "/xyz/openbmc_project/sensors/utilization"}},
             {node::thermal,
              {"/xyz/openbmc_project/sensors/fan_tach",
               "/xyz/openbmc_project/sensors/temperature",
               "/xyz/openbmc_project/sensors/fan_pwm"}}};
} // namespace dbus
} // namespace sensors

/**
 * SensorsAsyncResp
 * Gathers data needed for response processing after async calls are done
 */
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
                     const std::string& id, const std::string_view& node,
                     const std::vector<const char*>& prefixes = {}) :
        asyncResp(asyncResp),
        id{id}, node{node}, prefixes{prefixes}
    {}

    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(DataCompleteCb&& creationComplete,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& id, const std::string_view& node,
                     const std::vector<const char*>& prefixes = {}) :
        asyncResp(asyncResp),
        id{id}, node{node}, prefixes{prefixes},
        metadata{std::vector<SensorData>()}, dataComplete{
                                                 std::move(creationComplete)}
    {}

    ~SensorsAsyncResp()
    {
        if (asyncResp->res.result() ==
            boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            asyncResp->res.jsonValue = nlohmann::json::object();
        }

        if (dataComplete && metadata)
        {
            boost::container::flat_map<std::string, std::string> map;
            if (asyncResp->res.result() == boost::beast::http::status::ok)
            {
                for (auto& sensor : *metadata)
                {
                    map.insert(std::make_pair(sensor.uri + sensor.valueKey,
                                              sensor.dbusPath));
                }
            }
            dataComplete(asyncResp->res.result(), map);
        }
    }

    void addMetadata(const std::string& name, const std::string& objectUri,
                     const std::string& valueKey, const std::string& dbusPath)
    {
        if (metadata)
        {
            metadata->emplace_back(
                SensorData{name, objectUri, valueKey, dbusPath});
        }
    }

    void updateUri(const std::string& name, const std::string& uri)
    {
        if (metadata)
        {
            for (auto& sensor : *metadata)
            {
                if (sensor.name == name)
                {
                    sensor.uri = uri;
                }
            }
        }
    }

    const std::shared_ptr<bmcweb::AsyncResp> asyncResp;

    std::string id;
    std::string node;
    std::vector<const char*> prefixes;

  private:
    std::optional<std::vector<SensorData>> metadata;
    DataCompleteCb dataComplete;
};

inline void
    getChassisData(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp);

} // namespace redfish