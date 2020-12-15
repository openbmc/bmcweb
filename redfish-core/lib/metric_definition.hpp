#pragma once

#include "async_resp.hpp"
#include "node.hpp"
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

class AsyncRespWithFinalizer
{
  public:
    AsyncRespWithFinalizer(crow::Response& res) : asyncResp(res)
    {}

    AsyncRespWithFinalizer(crow::Response& res,
                           std::function<void(crow::Response&)> finalizer) :
        asyncResp(res),
        finalizer(std::move(finalizer))
    {}

    ~AsyncRespWithFinalizer()
    {
        if (finalizer)
        {
            try
            {
                finalizer(asyncResp.res);
            }
            catch (const std::exception& e)
            {
                BMCWEB_LOG_DEBUG << "Executing finalizer failed: " << e.what();
                messages::internalError(asyncResp.res);
            }
        }
    }

    AsyncRespWithFinalizer(const AsyncRespWithFinalizer&) = delete;
    AsyncRespWithFinalizer(AsyncRespWithFinalizer&&) = delete;

    void setFinalizer(std::function<void(crow::Response&)> newFinalizer)
    {
        finalizer = std::move(newFinalizer);
    }

    crow::Response& res()
    {
        return asyncResp.res;
    }

  private:
    AsyncResp asyncResp;
    std::function<void(crow::Response&)> finalizer;
};

template <typename F>
inline void getChassisNames(
    F&& cb, const std::shared_ptr<utils::AsyncRespWithFinalizer>& asyncResp)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [asyncResp,
         callback = std::move(cb)](const boost::system::error_code ec,
                                   const std::vector<std::string>& chassis) {
            std::vector<std::string> chassisNames;

            if (ec)
            {
                callback(ec, chassisNames);
                return;
            }

            chassisNames.reserve(chassis.size());
            for (const std::string& path : chassis)
            {
                sdbusplus::message::object_path dbusPath = path;
                std::string name = dbusPath.filename();
                if (name.empty())
                {
                    callback(boost::system::errc::make_error_code(
                                 boost::system::errc::invalid_argument),
                             chassisNames);
                    return;
                }
                chassisNames.emplace_back(std::move(name));
            }

            callback(ec, chassisNames);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

} // namespace utils

namespace telemetry
{

void addMembers(utils::AsyncRespWithFinalizer& asyncResp,
                const boost::container::flat_map<std::string, std::string>& el)
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

        nlohmann::json& members = asyncResp.res().jsonValue["Members"];

        const std::string odataId =
            telemetry::metricDefinitionUri + std::move(type);

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

        asyncResp.res().jsonValue["Members@odata.count"] = members.size();
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

        auto asyncResp = std::make_shared<utils::AsyncRespWithFinalizer>(res);

        auto handleRetrieveUriToDbusMap =
            [asyncResp](
                const boost::beast::http::status status,
                const boost::container::flat_map<std::string, std::string>&
                    uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                        "sensors map with err "
                                     << static_cast<unsigned>(status);
                    messages::internalError(asyncResp->res());
                    return;
                }
                telemetry::addMembers(*asyncResp, uriToDbus);
            };

        utils::getChassisNames(
            [handleRetrieveUriToDbusMap = std::move(handleRetrieveUriToDbusMap),
             asyncResp](boost::system::error_code ec,
                        const std::vector<std::string>& chassisNames) {
                if (ec)
                {
                    messages::internalError(asyncResp->res());
                    BMCWEB_LOG_DEBUG << "getChassisNames error: " << ec.value();
                    return;
                }

                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, _] : sensors::dbus::paths)
                    {
                        BMCWEB_LOG_INFO << "Chassis: " << chassisName
                                        << " sensor: " << sensorNode;
                        retrieveUriToDbusMap(chassisName, sensorNode.data(),
                                             handleRetrieveUriToDbusMap);
                    }
                }
            },
            asyncResp);
    }
};

namespace telemetry
{

void addMetricProperty(
    utils::AsyncRespWithFinalizer& asyncResp, const std::string& id,
    const boost::container::flat_map<std::string, std::string>& el)
{
    nlohmann::json& metricProperties =
        asyncResp.res().jsonValue["MetricProperties"];

    for (const auto& [redfishSensor, dbusSensor] : el)
    {
        std::string sensorId;
        if (dbus::utility::getNthStringFromPath(dbusSensor, 3, sensorId))
        {
            if (sensorId == id)
            {
                metricProperties.push_back(redfishSensor);
            }
        }
    }
}

} // namespace telemetry

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
        auto asyncResp = std::make_shared<utils::AsyncRespWithFinalizer>(res);

        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res());
            return;
        }

        const std::string& id = params[0];
        asyncResp->setFinalizer([id](crow::Response& res) {
            if (res.jsonValue["MetricProperties"].empty())
            {
                messages::resourceNotFound(res, "MetricDefinition", id);
            }
        });

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

        auto handleRetrieveUriToDbusMap =
            [asyncResp,
             id](const boost::beast::http::status status,
                 const boost::container::flat_map<std::string, std::string>&
                     uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                        "sensors map with err "
                                     << static_cast<unsigned>(status);
                    messages::internalError(asyncResp->res());
                    return;
                }
                telemetry::addMetricProperty(*asyncResp, id, uriToDbus);
            };

        utils::getChassisNames(
            [handleRetrieveUriToDbusMap = std::move(handleRetrieveUriToDbusMap),
             asyncResp, id](boost::system::error_code ec,
                            const std::vector<std::string>& chassisNames) {
                if (ec)
                {
                    messages::internalError(asyncResp->res());
                    BMCWEB_LOG_DEBUG << "getChassisNames error: " << ec.value();
                    return;
                }

                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, dbusPaths] :
                         sensors::dbus::paths)
                    {
                        retrieveUriToDbusMap(chassisName, sensorNode.data(),
                                             handleRetrieveUriToDbusMap);
                    }
                }
            },
            asyncResp);
    }
};

} // namespace redfish
