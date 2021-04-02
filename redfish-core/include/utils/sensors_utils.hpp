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

inline const char* toReadingType(const std::string& sensorType)
{
    if (sensorType == "voltage")
    {
        return "Voltage";
    }
    if (sensorType == "power")
    {
        return "Power";
    }
    if (sensorType == "current")
    {
        return "Current";
    }
    if (sensorType == "fan_tach")
    {
        return "Rotational";
    }
    if (sensorType == "temperature")
    {
        return "Temperature";
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization")
    {
        return "Percent";
    }
    if (sensorType == "altitude")
    {
        return "Altitude";
    }
    if (sensorType == "airflow")
    {
        return "AirFlow";
    }
    if (sensorType == "energy")
    {
        return "EnergyJoules";
    }
    return "";
}

inline const char* toReadingUnits(const std::string& sensorType)
{
    if (sensorType == "voltage")
    {
        return "V";
    }
    if (sensorType == "power")
    {
        return "W";
    }
    if (sensorType == "current")
    {
        return "A";
    }
    if (sensorType == "fan_tach")
    {
        return "RPM";
    }
    if (sensorType == "temperature")
    {
        return "Cel";
    }
    if (sensorType == "fan_pwm" || sensorType == "utilization")
    {
        return "%";
    }
    if (sensorType == "altitude")
    {
        return "m";
    }
    if (sensorType == "airflow")
    {
        return "cft_i/min";
    }
    if (sensorType == "energy")
    {
        return "J";
    }
    return "";
}

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

    SensorsAsyncResp(crow::Response& response, const std::string& id,
                     const std::string_view& node) :
        res(response),
        id{id}, node{node}
    {}

    // Store extra data about sensor mapping and return it in callback
    SensorsAsyncResp(crow::Response& response, const std::string& id,
                     const std::string_view& node,
                     DataCompleteCb&& creationComplete) :
        res(response),
        id{id}, node{node}, metadata{std::vector<SensorData>()},
        dataComplete{std::move(creationComplete)}
    {}

    ~SensorsAsyncResp()
    {
        if (res.result() == boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            res.jsonValue = nlohmann::json::object();
        }

        if (dataComplete && metadata)
        {
            boost::container::flat_map<std::string, std::string> map;
            if (res.result() == boost::beast::http::status::ok)
            {
                for (auto& sensor : *metadata)
                {
                    map.insert(std::make_pair(sensor.uri + sensor.valueKey,
                                              sensor.dbusPath));
                }
            }
            dataComplete(res.result(), map);
        }

        res.end();
    }

    void addMetadata(const std::string& name, const std::string& objectUri,
                     const std::string valueKey, const std::string& dbusPath)
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

    crow::Response& res;

    std::string id;
    std::string node;
    std::optional<std::vector<const char*>> filter;

  private:
    std::optional<std::vector<SensorData>> metadata;
    DataCompleteCb dataComplete;
};

inline void
    getChassisData(const std::shared_ptr<SensorsAsyncResp>& sensorsAsyncResp);

/**
 * @brief Retrieves mapping of Redfish URIs to sensor value property to D-Bus
 * path of the sensor.
 *
 * Function builds valid Redfish response for sensor query of given chassis and
 * node. It then builds metadata about Redfish<->D-Bus correlations and
 * provides it to caller in a callback.
 *
 * @param id   Resource id for which retrieval should be performed
 * @param node  Node (group) of sensors. See sensors::node for supported
 * values
 * @param mapComplete   Callback to be called with retrieval result
 */
inline void retrieveUriToDbusMap(const std::string& id, const std::string& node,
                                 SensorsAsyncResp::DataCompleteCb&& mapComplete)
{
    auto typesIt = sensors::dbus::paths.find(node);
    if (typesIt == sensors::dbus::paths.end())
    {
        BMCWEB_LOG_ERROR << "Wrong node provided : " << node;
        mapComplete(boost::beast::http::status::bad_request, {});
        return;
    }

    auto respBuffer = std::make_shared<crow::Response>();
    auto callback =
        [respBuffer, mapCompleteCb{std::move(mapComplete)}](
            const boost::beast::http::status status,
            const boost::container::flat_map<std::string, std::string>&
                uriToDbus) { mapCompleteCb(status, uriToDbus); };
    auto resp = std::make_shared<SensorsAsyncResp>(*respBuffer, id, node,
                                                   std::move(callback));
    resp->filter = typesIt->second;
    getChassisData(resp);
}

} // namespace redfish