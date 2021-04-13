#pragma once

#include "async_resp.hpp"
#include "node.hpp"
#include "sensors.hpp"
#include "utils/get_chassis_names.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace telemetry
{

void addMembers(crow::Response& res,
                const boost::container::flat_map<std::string, std::string>& el)
{
    for (const auto& [_, dbusSensor] : el)
    {
        sdbusplus::message::object_path path(dbusSensor);
        sdbusplus::message::object_path parentPath = path.parent_path();
        const std::string type = parentPath.filename();

        if (type.empty())
        {
            BMCWEB_LOG_ERROR << "Received invalid DBus Sensor Path = "
                             << dbusSensor;
            continue;
        }

        nlohmann::json& members = res.jsonValue["Members"];

        const std::string odataId =
            std::string(telemetry::metricDefinitionUri) +
            sensors::toReadingType(type);

        const auto it = std::find_if(members.begin(), members.end(),
                                     [&odataId](const nlohmann::json& item) {
                                         auto kt = item.find("@odata.id");
                                         if (kt == item.end())
                                         {
                                             return false;
                                         }
                                         const std::string* value =
                                             kt->get_ptr<const std::string*>();
                                         if (!value)
                                         {
                                             return false;
                                         }
                                         return *value == odataId;
                                     });

        if (it == members.end())
        {
            members.push_back({{"@odata.id", odataId}});
        }

        res.jsonValue["Members@odata.count"] = members.size();
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&, const std::vector<std::string>&) override
    {
        asyncResp->res.jsonValue["@odata.type"] = "#MetricDefinitionCollection."
                                                  "MetricDefinitionCollection";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/TelemetryService/MetricDefinitions";
        asyncResp->res.jsonValue["Name"] = "Metric Definition Collection";
        asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Members@odata.count"] = 0;

        utils::getChassisNames(
            [asyncResp](boost::system::error_code ec,
                        const std::vector<std::string>& chassisNames) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "getChassisNames error: " << ec.value();
                    return;
                }

                auto handleRetrieveUriToDbusMap =
                    [asyncResp](const boost::beast::http::status status,
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
                        telemetry::addMembers(asyncResp->res, uriToDbus);
                    };

                for (const std::string& chassisName : chassisNames)
                {
                    for (const auto& [sensorNode, _] : sensors::dbus::paths)
                    {
                        BMCWEB_LOG_DEBUG << "Chassis: " << chassisName
                                         << " sensor: " << sensorNode;
                        retrieveUriToDbusMap(chassisName, sensorNode.data(),
                                             handleRetrieveUriToDbusMap);
                    }
                }
            });
    }
};

namespace telemetry
{

bool isSensorIdSupported(std::string_view readingType)
{
    for (const std::pair<std::string_view, std::vector<const char*>>&
             typeToPaths : sensors::dbus::paths)
    {
        for (const char* supportedPath : typeToPaths.second)
        {
            if (readingType ==
                sensors::toReadingType(
                    sdbusplus::message::object_path(supportedPath).filename()))
            {
                return true;
            }
        }
    }
    return false;
}

void addMetricProperty(
    bmcweb::AsyncResp& asyncResp, const std::string& readingType,
    const boost::container::flat_map<std::string, std::string>& el)
{
    nlohmann::json& metricProperties =
        asyncResp.res.jsonValue["MetricProperties"];

    for (const auto& [redfishSensor, dbusSensor] : el)
    {
        std::string sensorId;
        if (dbus::utility::getNthStringFromPath(dbusSensor, 3, sensorId))
        {
            if (sensors::toReadingType(sensorId) == readingType)
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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& readingType = params[0];

        if (!telemetry::isSensorIdSupported(readingType))
        {
            messages::resourceNotFound(asyncResp->res, "MetricDefinition",
                                       readingType);
            return;
        }

        asyncResp->res.jsonValue["MetricProperties"] = nlohmann::json::array();
        asyncResp->res.jsonValue["Id"] = readingType;
        asyncResp->res.jsonValue["Name"] = readingType;
        asyncResp->res.jsonValue["@odata.id"] =
            telemetry::metricDefinitionUri + readingType;
        asyncResp->res.jsonValue["@odata.type"] =
            "#MetricDefinition.v1_0_3.MetricDefinition";
        asyncResp->res.jsonValue["MetricDataType"] = "Decimal";
        asyncResp->res.jsonValue["MetricType"] = "Numeric";
        asyncResp->res.jsonValue["IsLinear"] = true;
        asyncResp->res.jsonValue["Units"] =
            sensors::toReadingUnits(readingType);

        utils::getChassisNames([asyncResp,
                                readingType](boost::system::error_code ec,
                                             const std::vector<std::string>&
                                                 chassisNames) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR << "getChassisNames error: " << ec.value();
                return;
            }

            auto handleRetrieveUriToDbusMap =
                [asyncResp, readingType](
                    const boost::beast::http::status status,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
                    if (status != boost::beast::http::status::ok)
                    {
                        BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                            "sensors map with err "
                                         << static_cast<unsigned>(status);
                        messages::internalError(asyncResp->res);
                        return;
                    }
                    telemetry::addMetricProperty(*asyncResp, readingType,
                                                 uriToDbus);
                };

            for (const std::string& chassisName : chassisNames)
            {
                for (const auto& [sensorNode, dbusPaths] : sensors::dbus::paths)
                {
                    retrieveUriToDbusMap(chassisName, sensorNode.data(),
                                         handleRetrieveUriToDbusMap);
                }
            }
        });
    }
};

} // namespace redfish
