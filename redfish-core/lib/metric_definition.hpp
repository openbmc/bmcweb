#pragma once

#include "async_resp.hpp"
#include "sensors.hpp"
#include "utils/get_chassis_names.hpp"
#include "utils/telemetry_utils.hpp"

#include <registries/privilege_registry.hpp>

namespace redfish
{

namespace telemetry
{

struct ValueVisitor
{
    ValueVisitor(boost::system::error_code& ec) : ec(ec)
    {}

    template <class T>
    double operator()(T value) const
    {
        return static_cast<double>(value);
    }

    double operator()(std::monostate) const
    {
        ec = boost::system::errc::make_error_code(
            boost::system::errc::invalid_argument);
        return double{};
    }

    boost::system::error_code& ec;
};

inline void getReadingRange(
    const std::string& service, const std::string& path,
    const std::string& property,
    std::function<void(boost::system::error_code, double)> callback)
{
    crow::connections::systemBus->async_method_call(
        [callback = std::move(callback)](
            boost::system::error_code ec,
            const std::variant<std::monostate, double, uint64_t, int64_t,
                               uint32_t, int32_t, uint16_t, int16_t>&
                valueVariant) {
            if (ec)
            {
                callback(ec, double{});
                return;
            }

            const double value = std::visit(ValueVisitor(ec), valueVariant);

            callback(ec, value);
        },
        service, path, "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Sensor.Value", property);
}

inline void
    fillMinMaxReadingRange(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& serviceName,
                           const std::string& sensorPath)
{
    asyncResp->res.jsonValue["MetricType"] = "Numeric";

    telemetry::getReadingRange(
        serviceName, sensorPath, "MinValue",
        [asyncResp](boost::system::error_code ec, double readingRange) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (std::isfinite(readingRange))
            {
                asyncResp->res.jsonValue["MetricType"] = "Gauge";

                asyncResp->res.jsonValue["MinReadingRange"] = readingRange;
            }
        });

    telemetry::getReadingRange(
        serviceName, sensorPath, "MaxValue",
        [asyncResp](boost::system::error_code ec, double readingRange) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (std::isfinite(readingRange))
            {
                asyncResp->res.jsonValue["MetricType"] = "Gauge";

                asyncResp->res.jsonValue["MaxReadingRange"] = readingRange;
            }
        });
}

inline void getSensorService(
    const std::string& sensorPath,
    std::function<void(boost::system::error_code, const std::string&)> callback)
{
    using ResultType = std::pair<
        std::string,
        std::vector<std::pair<std::string, std::vector<std::string>>>>;

    crow::connections::systemBus->async_method_call(
        [sensorPath, callback = std::move(callback)](
            boost::system::error_code ec,
            const std::vector<ResultType>& result) {
            if (ec)
            {
                callback(ec, std::string{});
                return;
            }

            for (const auto& [path, serviceToInterfaces] : result)
            {
                if (path == sensorPath)
                {
                    for (const auto& [service, interfaces] :
                         serviceToInterfaces)
                    {
                        callback(boost::system::errc::make_error_code(
                                     boost::system::errc::success),
                                 service);
                        return;
                    }
                }
            }

            callback(boost::system::errc::make_error_code(
                         boost::system::errc::no_such_file_or_directory),
                     std::string{});
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2,
        std::array{"xyz.openbmc_project.Sensor.Value"});
}

constexpr auto metricDefinitionMapping = std::array{
    std::pair{"fan_pwm", "Fan_Pwm"}, std::pair{"fan_tach", "Fan_Tach"}};

std::string mapSensorToMetricDefinition(const std::string& sensorPath)
{
    sdbusplus::message::object_path sensorObjectPath{sensorPath};

    const auto it = std::find_if(
        metricDefinitionMapping.begin(), metricDefinitionMapping.end(),
        [&sensorObjectPath](const auto& item) {
            return item.first == sensorObjectPath.parent_path().filename();
        });

    const char* metricDefinitionPath =
        "/redfish/v1/TelemetryService/MetricDefinitions/";

    if (it != metricDefinitionMapping.end())
    {
        return std::string{metricDefinitionPath} + it->second;
    }

    return metricDefinitionPath + sensorObjectPath.filename();
}

template <class Callback>
inline void mapRedfishUriToDbusPath(Callback&& callback)
{
    utils::getChassisNames([callback = std::move(callback)](
                               boost::system::error_code ec,
                               const std::vector<std::string>& chassisNames) {
        if (ec)
        {
            BMCWEB_LOG_ERROR << "getChassisNames error: " << ec.value();
            callback(ec, {});
            return;
        }

        auto counter = std::make_shared<std::pair<
            boost::container::flat_map<std::string, std::string>, size_t>>();

        auto handleRetrieveUriToDbusMap =
            [counter, callback = std::move(callback)](
                const boost::beast::http::status status,
                const boost::container::flat_map<std::string, std::string>&
                    uriToDbus) {
                if (status != boost::beast::http::status::ok)
                {
                    BMCWEB_LOG_ERROR << "Failed to retrieve URI to dbus "
                                        "sensors map with err "
                                     << static_cast<unsigned>(status);
                    counter->second = 0u;
                    callback(boost::system::errc::make_error_code(
                                 boost::system::errc::io_error),
                             {});
                    return;
                }

                for (const auto& [key, value] : uriToDbus)
                {
                    counter->first[key] = value;
                }

                if (--counter->second == 0u)
                {
                    callback(boost::system::errc::make_error_code(
                                 boost::system::errc::success),
                             counter->first);
                }
            };

        for (const std::string& chassisName : chassisNames)
        {
            for (const auto& [sensorNode, dbusPaths] : sensors::dbus::paths)
            {
                ++counter->second;
                retrieveUriToDbusMap(chassisName, sensorNode.data(),
                                     handleRetrieveUriToDbusMap);
            }
        }
    });
}

} // namespace telemetry

inline void requestRoutesMetricDefinitionCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricDefinitions/")
        .privileges(privileges::getTelemetryService)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                telemetry::mapRedfishUriToDbusPath(
                    [asyncResp](boost::system::error_code ec,
                                const boost::container::flat_map<
                                    std::string, std::string>& uriToDbus) {
                        if (ec)
                        {
                            messages::internalError(asyncResp->res);
                            BMCWEB_LOG_ERROR
                                << "mapRedfishUriToDbusPath error: "
                                << ec.value();
                            return;
                        }

                        std::set<std::string> members;

                        for (const auto& [uri, dbusPath] : uriToDbus)
                        {
                            members.insert(
                                telemetry::mapSensorToMetricDefinition(
                                    dbusPath));
                        }

                        for (const std::string& odataId : members)
                        {
                            asyncResp->res.jsonValue["Members"].push_back(
                                {{"@odata.id", odataId}});
                        }

                        asyncResp->res.jsonValue["Members@odata.count"] =
                            asyncResp->res.jsonValue["Members"].size();
                    });

                asyncResp->res.jsonValue["@odata.type"] =
                    "#MetricDefinitionCollection."
                    "MetricDefinitionCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/TelemetryService/MetricDefinitions";
                asyncResp->res.jsonValue["Name"] =
                    "Metric Definition Collection";
                asyncResp->res.jsonValue["Members"] = nlohmann::json::array();
                asyncResp->res.jsonValue["Members@odata.count"] = 0;
            });
}

inline void requestRoutesMetricDefinition(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/TelemetryService/MetricDefinitions/<str>/")
        .privileges(privileges::getTelemetryService)
        .methods(
            boost::beast::http::verb::get)([](const crow::Request&,
                                              const std::shared_ptr<
                                                  bmcweb::AsyncResp>& asyncResp,
                                              const std::string& name) {
            telemetry::mapRedfishUriToDbusPath(
                [asyncResp, name](
                    boost::system::error_code ec,
                    const boost::container::flat_map<std::string, std::string>&
                        uriToDbus) {
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        BMCWEB_LOG_ERROR << "mapRedfishUriToDbusPath error: "
                                         << ec.value();
                        return;
                    }

                    std::string odataId = telemetry::metricDefinitionUri + name;
                    boost::container::flat_map<std::string, std::string>
                        matchingUris;

                    for (const auto& [uri, dbusPath] : uriToDbus)
                    {
                        if (telemetry::mapSensorToMetricDefinition(dbusPath) ==
                            odataId)
                        {
                            matchingUris.emplace(uri, dbusPath);
                        }
                    }

                    if (matchingUris.empty())
                    {
                        messages::resourceNotFound(asyncResp->res,
                                                   "MetricDefinition", name);
                        return;
                    }

                    std::string sensorPath = matchingUris.begin()->second;

                    telemetry::getSensorService(
                        sensorPath,
                        [asyncResp, name, odataId = std::move(odataId),
                         sensorPath, matchingUris = std::move(matchingUris)](
                            boost::system::error_code ec,
                            const std::string& serviceName) {
                            if (ec)
                            {
                                messages::internalError(asyncResp->res);
                                BMCWEB_LOG_ERROR << "getServiceSensorFailed: "
                                                 << ec.value();
                                return;
                            }

                            asyncResp->res.jsonValue["Id"] = name;
                            asyncResp->res.jsonValue["Name"] = name;
                            asyncResp->res.jsonValue["@odata.id"] = odataId;
                            asyncResp->res.jsonValue["@odata.type"] =
                                "#MetricDefinition.v1_0_3.MetricDefinition";
                            asyncResp->res.jsonValue["MetricDataType"] =
                                "Decimal";
                            asyncResp->res.jsonValue["IsLinear"] = true;
                            asyncResp->res.jsonValue["Units"] =
                                sensors::toReadingUnits(
                                    sdbusplus::message::object_path{sensorPath}
                                        .parent_path()
                                        .filename());

                            for (const auto& [uri, dbusPath] : matchingUris)
                            {
                                asyncResp->res.jsonValue["MetricProperties"]
                                    .push_back(uri);
                            }

                            telemetry::fillMinMaxReadingRange(
                                asyncResp, serviceName, sensorPath);
                        });
                });
        });
}

} // namespace redfish
