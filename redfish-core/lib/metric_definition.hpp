#pragma once

#include "node.hpp"
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

template <typename F>
inline void getChassisNames(F&& cb, const std::shared_ptr<AsyncResp>& asyncResp)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         callback = std::move(cb)](const boost::system::error_code ec,
                                   std::vector<std::string>& chassis) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_DEBUG << "DBus call error: " << ec.value();
                return;
            }

            std::vector<std::string> chassisNames;
            chassisNames.reserve(chassis.size());
            for (sdbusplus::message::object_path path : chassis)
            {
                std::string name = path.filename();
                if (name.empty())
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "Invalid chassis: " << path.str;
                    return;
                }
                chassisNames.emplace_back(std::move(name));
            }

            callback(chassisNames);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}
} // namespace utils

namespace telemetry
{

class MetricDefinitionCollectionReduce
{
  public:
    MetricDefinitionCollectionReduce(
        const std::shared_ptr<AsyncResp>& asyncResp) :
        asyncResp{asyncResp}
    {}

    ~MetricDefinitionCollectionReduce()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }

        nlohmann::json& members = asyncResp->res.jsonValue["Members"];
        members = nlohmann::json::array();

        for (const std::string& type : dbusTypes)
        {
            members.push_back(
                {{"@odata.id", telemetry::metricDefinitionUri + type}});
        }
        asyncResp->res.jsonValue["Members@odata.count"] = members.size();
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        for (const auto& [_, dbusSensor] : el)
        {
            sdbusplus::message::object_path path(dbusSensor);
            sdbusplus::message::object_path parentPath = path.parent_path();
            std::string type = parentPath.filename();
            if (type.empty())
            {
                BMCWEB_LOG_ERROR << "Received invalid DBus Sensor Path = "
                                 << dbusSensor;
                continue;
            }

            dbusTypes.insert(std::move(type));
        }
    }

  private:
    const std::shared_ptr<AsyncResp> asyncResp;
    boost::container::flat_set<std::string> dbusTypes;
};

class MetricDefinitionReduce
{
  public:
    MetricDefinitionReduce(const std::shared_ptr<AsyncResp>& asyncResp,
                           const std::string& id) :
        id(id),
        pattern{'/' + id + '/'}, asyncResp{asyncResp}
    {}
    ~MetricDefinitionReduce()
    {
        if (asyncResp->res.result() != boost::beast::http::status::ok)
        {
            return;
        }
        if (redfishSensors.empty())
        {
            messages::resourceNotFound(asyncResp->res, "MetricDefinition", id);
            return;
        }

        asyncResp->res.jsonValue["MetricProperties"] = redfishSensors;
        asyncResp->res.jsonValue["Id"] = id;
        asyncResp->res.jsonValue["Name"] = id;
        asyncResp->res.jsonValue["@odata.id"] =
            telemetry::metricDefinitionUri + id;
        asyncResp->res.jsonValue["@odata.type"] =
            "#MetricDefinition.v1_0_3.MetricDefinition";
        asyncResp->res.jsonValue["MetricDataType"] = "Decimal";
        asyncResp->res.jsonValue["MetricType"] = "Numeric";
        asyncResp->res.jsonValue["IsLinear"] = true;
        asyncResp->res.jsonValue["Units"] = sensors::toReadingUnits(id);
    }

    void insert(const boost::container::flat_map<std::string, std::string>& el)
    {
        for (const auto& [redfishSensor, dbusSensor] : el)
        {
            if (dbusSensor.find(pattern) != std::string::npos)
            {
                redfishSensors.push_back(redfishSensor);
            }
        }
    }

  private:
    const std::string id;
    const std::string pattern;
    const std::shared_ptr<AsyncResp> asyncResp;
    std::vector<std::string> redfishSensors;
};
} // namespace telemetry

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
        utils::getChassisNames(
            [asyncResp](const std::vector<std::string>& chassisNames) {
                auto collectionReduce = std::make_shared<
                    telemetry::MetricDefinitionCollectionReduce>(asyncResp);
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
            },
            asyncResp);
    }
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
        utils::getChassisNames(
            [asyncResp, id](const std::vector<std::string>& chassisNames) {
                auto definitionGather =
                    std::make_shared<telemetry::MetricDefinitionReduce>(
                        asyncResp, id);
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
            },
            asyncResp);
    }
};

} // namespace redfish
