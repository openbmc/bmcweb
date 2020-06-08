#pragma once

#include "node.hpp"
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

template <typename F>
inline void getChassisNames(F&& cb)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [callback = std::move(cb)](const boost::system::error_code ec,
                                   std::vector<std::string>& chassisList) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG << "DBus call error: " << ec.value();
                return;
            }

            std::vector<std::string> chassisNames;
            chassisNames.reserve(chassisList.size());
            for (const std::string& chassisPath : chassisList)
            {
                size_t pos = chassisPath.rfind("/");
                if (pos == std::string::npos)
                {
                    continue;
                }
                chassisNames.push_back(chassisPath.substr(pos + 1));
            }

            callback(chassisNames);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}
} // namespace utils

class MetricDefinitionCollection : public Node
{
  public:
    MetricDefinitionCollection(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricDefinitions/")
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>&) override
    {
        res.jsonValue["@odata.type"] = "#MetricDefinitionCollection."
                                       "MetricDefinitionCollection";
        res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricDefinitions";
        res.jsonValue["Name"] = "Metric Definition Collection";
        res.jsonValue["Members"] = nlohmann::json::array();
        res.jsonValue["Members@odata.count"] = 0;

        auto asyncResp = std::make_shared<AsyncResp>(res);
        auto collectionReduce = std::make_shared<CollectionGather>(asyncResp);
        utils::getChassisNames(
            [asyncResp,
             collectionReduce](const std::vector<std::string>& chassisNames) {
                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, _] : sensors::dbus::paths)
                    {
                        BMCWEB_LOG_INFO << "Chassis: " << chassisName
                                        << " sensor: " << sensorNode;
                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp, collectionReduce](
                                const boost::beast::http::status status,
                                const boost::container::flat_map<
                                    std::string, std::string>& uriToDbus) {
                                if (status != boost::beast::http::status::ok)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Failed to retrieve URI to dbus "
                                           "sensors map with err "
                                        << static_cast<unsigned>(status);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                collectionReduce->insert(uriToDbus);
                            });
                    }
                }
            });
    }

    class CollectionGather
    {
      public:
        CollectionGather(const std::shared_ptr<AsyncResp>& asyncResp) :
            asyncResp{asyncResp}
        {}

        ~CollectionGather()
        {
            if (asyncResp->res.result() != boost::beast::http::status::ok)
            {
                return;
            }

            telemetry::dbusPathsToMembers(
                asyncResp,
                std::vector<std::string>(dbusTypes.begin(), dbusTypes.end()),
                telemetry::metricDefinitionUri);
        }

        void insert(
            const boost::container::flat_map<std::string, std::string>& el)
        {
            for (const auto& [_, dbusSensor] : el)
            {
                size_t pos = dbusSensor.rfind("/");
                if (pos == std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Received invalid DBus Sensor Path = "
                                     << dbusSensor;
                    continue;
                }

                dbusTypes.insert(dbusSensor.substr(0, pos));
            }
        }

      private:
        const std::shared_ptr<AsyncResp> asyncResp;
        boost::container::flat_set<std::string> dbusTypes;
    };
};

class MetricDefinition : public Node
{
  public:
    MetricDefinition(App& app) :
        Node(app, "/redfish/v1/TelemetryService/MetricDefinitions/<str>/",
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
    void doGet(crow::Response& res, const crow::Request&,
               const std::vector<std::string>& params) override
    {
        auto asyncResp = std::make_shared<AsyncResp>(res);
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];
        auto definitionGather =
            std::make_shared<DefinitionGather>(asyncResp, id);
        utils::getChassisNames(
            [asyncResp,
             definitionGather](const std::vector<std::string>& chassisNames) {
                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, dbusPaths] :
                         sensors::dbus::paths)
                    {
                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp, definitionGather](
                                const boost::beast::http::status status,
                                const boost::container::flat_map<
                                    std::string, std::string>& uriToDbus) {
                                if (status != boost::beast::http::status::ok)
                                {
                                    BMCWEB_LOG_ERROR
                                        << "Failed to retrieve URI to dbus "
                                           "sensors map with err "
                                        << static_cast<unsigned>(status);
                                    messages::internalError(asyncResp->res);
                                    return;
                                }
                                definitionGather->insert(uriToDbus);
                            });
                    }
                }
            });
    }

    class DefinitionGather
    {
      public:
        DefinitionGather(const std::shared_ptr<AsyncResp>& asyncResp,
                         const std::string& id) :
            id(id),
            pattern{'/' + id + '/'}, asyncResp{asyncResp}
        {}
        ~DefinitionGather()
        {
            if (asyncResp->res.result() != boost::beast::http::status::ok)
            {
                return;
            }
            if (redfishSensors.empty())
            {
                messages::resourceNotFound(asyncResp->res, schemaType, id);
                return;
            }

            asyncResp->res.jsonValue["MetricProperties"] = redfishSensors;
            asyncResp->res.jsonValue["Id"] = id;
            asyncResp->res.jsonValue["Name"] = id;
            asyncResp->res.jsonValue["@odata.id"] =
                telemetry::metricDefinitionUri + id;
            asyncResp->res.jsonValue["@odata.type"] = schemaType;
            asyncResp->res.jsonValue["MetricDataType"] = "Decimal";
            asyncResp->res.jsonValue["MetricType"] = "Numeric";
            asyncResp->res.jsonValue["IsLinear"] = true;
            asyncResp->res.jsonValue["Units"] = sensors::toReadingUnits(id);
        }

        void insert(
            const boost::container::flat_map<std::string, std::string>& el)
        {
            for (const auto& [redfishSensor, dbusSensor] : el)
            {
                if (dbusSensor.find(pattern) != std::string::npos)
                {
                    redfishSensors.push_back(redfishSensor);
                }
            }
        }

        const std::string id;
        const std::string pattern;

      private:
        const std::shared_ptr<AsyncResp> asyncResp;
        std::vector<std::string> redfishSensors;
    };

    static constexpr const char* schemaType =
        "#MetricDefinition.v1_0_3.MetricDefinition";
};

} // namespace redfish
