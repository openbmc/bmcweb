#pragma once

#include "async_resp.hpp"
#include "node.hpp"
#include "sensors.hpp"
#include "utils/finalizer.hpp"
#include "utils/get_inventory_items.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

template <typename F>
void retrieveMappings(
    F& handler,
    const boost::container::flat_map<std::string, utils::InventoryItemType>&
        inventoryItems)
{
    for (const auto& [item, type] : inventoryItems)
    {
        // Only Chassis sensors supported for now
        if (type == utils::InventoryItemType::chassis)
        {
            for (const auto& [sensorNode, dbusPaths] : sensors::dbus::paths)
            {
                retrieveUriToDbusMap(item, sensorNode.data(), handler);
            }
        }
    }
}

} // namespace utils

namespace telemetry
{

void addMembers(crow::Response& res,
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

        nlohmann::json& members = res.jsonValue["Members"];

        const std::string odataId =
            telemetry::metricDefinitionUri + std::move(type);

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

        utils::getInventoryItems(
            [asyncResp](
                boost::system::error_code ec,
                const boost::container::flat_map<
                    std::string, utils::InventoryItemType>& inventoryItems) {
                if (ec)
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "getInventoryItems error: "
                                     << ec.value();
                    return;
                }

                auto handleRetrieveMappings =
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

                utils::retrieveMappings(handleRetrieveMappings, inventoryItems);
            });
    }
};

namespace telemetry
{

void addMetricProperty(
    bmcweb::AsyncResp& asyncResp, const std::string& id,
    const boost::container::flat_map<std::string, std::string>& el)
{
    nlohmann::json& metricProperties =
        asyncResp.res.jsonValue["MetricProperties"];

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
    void doGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const crow::Request&,
               const std::vector<std::string>& params) override
    {
        if (params.size() != 1)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string& id = params[0];

        asyncResp->res.jsonValue["MetricProperties"] = nlohmann::json::array();
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

        auto finalizer = std::make_shared<utils::Finalizer>([asyncResp, id] {
            if (asyncResp->res.jsonValue["MetricProperties"].empty())
            {
                messages::resourceNotFound(asyncResp->res, "MetricDefinition",
                                           id);
            }
        });

        utils::getInventoryItems([asyncResp, finalizer, id](
                                     boost::system::error_code ec,
                                     const boost::container::flat_map<
                                         std::string, utils::InventoryItemType>&
                                         inventoryItems) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_ERROR << "getInventoryItems error: " << ec.value();
                return;
            }

            auto handleRetrieveMappings =
                [asyncResp, finalizer,
                 id](const boost::beast::http::status status,
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
                    telemetry::addMetricProperty(*asyncResp, id, uriToDbus);
                };

            utils::retrieveMappings(handleRetrieveMappings, inventoryItems);
        });
    }
};

} // namespace redfish
