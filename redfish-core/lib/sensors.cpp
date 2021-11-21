#include <variant>
#include <sdbusplus/message.hpp>
#include <boost/container/flat_set.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include "../../include/dbus_singleton.hpp"
#include "../../include/dbus_utility.hpp"
#include "sensors.hpp"
#include "../../http/logging.hpp"

namespace redfish
{

// Forward declaration of internalError
// Found in error_messages.hpp
namespace messages
{
    void internalError(crow::Response& res);
    void resourceNotFound(crow::Response& res, const std::string& arg1,
                      const std::string& arg2);
}

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant =
    std::variant<int64_t, double, uint32_t, bool, std::string>;

using ManagedObjectsVectorType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string, boost::container::flat_map<std::string, SensorVariant>>>>;

SensorsAsyncResp::SensorsAsyncResp(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& chassisIdIn,
                    const std::vector<const char*>& typesIn,
                    const std::string_view& subNode,
                    DataCompleteCb&& creationComplete) :
    asyncResp(asyncResp),
    chassisId(chassisIdIn), types(typesIn),
    chassisSubNode(subNode), metadata{std::vector<SensorData>()},
    dataComplete{std::move(creationComplete)}
{}

SensorsAsyncResp::~SensorsAsyncResp()
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

void SensorsAsyncResp::addMetadata(const nlohmann::json& sensorObject,
                    const std::string& valueKey, const std::string& dbusPath)
{
    if (metadata)
    {
        metadata->emplace_back(SensorData{sensorObject["Name"],
                                            sensorObject["@odata.id"],
                                            valueKey, dbusPath});
    }
}

void SensorsAsyncResp::updateUri(const std::string& name, const std::string& uri)
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

}