#pragma once

#include "node.hpp"
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

template <class T>
class Finalizer
{
  public:
    Finalizer(T&& callback) : callback(std::move(callback))
    {}

    ~Finalizer()
    {
        try
        {
            callback();
        }
        catch (const std::exception& e)
        {

            BMCWEB_LOG_ERROR << "Exception occured in ~Finalizer(): "
                             << e.what();
        }
    }

  private:
    T callback;
};

template <class T>
auto makeFinalizer(T&& callback)
{
    return std::make_shared<Finalizer<T>>(std::forward<T>(callback));
}

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
            for (const auto& path : chassis)
            {
                sdbusplus::message::object_path dbusPath = path;
                std::string name = dbusPath.filename();
                if (name.empty())
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "Invalid chassis: " << dbusPath.str;
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

void addMembers(AsyncResp& asyncResp,
                const boost::container::flat_map<std::string, std::string>& el)
{
    if (asyncResp.res.result() != boost::beast::http::status::ok)
    {
        return;
    }

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

        nlohmann::json& members = asyncResp.res.jsonValue["Members"];

        const auto odataId = telemetry::metricDefinitionUri + std::move(type);

        const auto it =
            std::find_if(members.begin(), members.end(),
                         [&odataId](const nlohmann::json& item) {
                             auto kt = item.find("@odata.id");
                             if (kt == item.end())
                             {
                                 return false;
                             }
                             return kt->get<std::string>() == odataId;
                         });

        if (it == members.end())
        {
            members.push_back({{"@odata.id", odataId}});
        }

        asyncResp.res.jsonValue["Members@odata.count"] = members.size();
    }
}

void addMetricProperty(
    AsyncResp& asyncResp, const std::string& id,
    const boost::container::flat_map<std::string, std::string>& el)
{
    if (asyncResp.res.result() != boost::beast::http::status::ok)
    {
        return;
    }

    nlohmann::json& metricProperties =
        asyncResp.res.jsonValue["MetricProperties"];

    for (const auto& [redfishSensor, dbusSensor] : el)
    {
        std::string sensorId;
        if (dbus::utility::getNthStringFromPath(dbusSensor, 3, sensorId) &&
            sensorId == id)
        {
            metricProperties.push_back(redfishSensor);
        }
    }
}

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
                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, _] : sensors::dbus::paths)
                    {
                        BMCWEB_LOG_INFO << "Chassis: " << chassisName
                                        << " sensor: " << sensorNode;
                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp](
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
                                telemetry::addMembers(*asyncResp, uriToDbus);
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
        res.jsonValue["MetricProperties"] = nlohmann::json::array();
        res.jsonValue["Id"] = id;
        res.jsonValue["Name"] = id;
        res.jsonValue["@odata.id"] = telemetry::metricDefinitionUri + id;
        res.jsonValue["@odata.type"] =
            "#MetricDefinition.v1_0_3.MetricDefinition";
        res.jsonValue["MetricDataType"] = "Decimal";
        res.jsonValue["MetricType"] = "Numeric";
        res.jsonValue["IsLinear"] = true;
        res.jsonValue["Units"] = sensors::toReadingUnits(id);

        utils::getChassisNames(
            [asyncResp, id](const std::vector<std::string>& chassisNames) {
                auto finalizer = utils::makeFinalizer([asyncResp, id] {
                    if (asyncResp->res.jsonValue["MetricProperties"].empty())
                    {
                        messages::resourceNotFound(asyncResp->res,
                                                   "MetricDefinition", id);
                    }
                });

                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, dbusPaths] :
                         sensors::dbus::paths)
                    {
                        retrieveUriToDbusMap(
                            chassisName, sensorNode.data(),
                            [asyncResp, id, finalizer](
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
                                telemetry::addMetricProperty(*asyncResp, id,
                                                             uriToDbus);
                            });
                    }
                }
            },
            asyncResp);
    }
};

} // namespace redfish
