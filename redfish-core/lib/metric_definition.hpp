#pragma once

#include "async_resp.hpp"
#include "node.hpp"
#include "sensors.hpp"
#include "utils/telemetry_utils.hpp"

namespace redfish
{

namespace utils
{

enum class InventoryItemType
{
    // TODO: Support more types when proper schemas will be implemented
    chassis
};

template <typename F>
inline void
    getInventoryNames(F&& cb,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Inventory.Item.Board",
        "xyz.openbmc_project.Inventory.Item.Chassis"};

    crow::connections::systemBus->async_method_call(
        [asyncResp, callback = std::move(cb)](
            const boost::system::error_code ec, const GetSubTreeType& tree) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                BMCWEB_LOG_DEBUG << "DBus call error: " << ec.value();
                return;
            }

            std::vector<std::pair<std::string, InventoryItemType>> itemNames;
            itemNames.reserve(tree.size());
            for (const auto& [itemPath, serviceIfaces] : tree)
            {
                sdbusplus::message::object_path path(itemPath);
                std::string name = path.filename();
                if (name.empty())
                {
                    messages::internalError(asyncResp->res);
                    BMCWEB_LOG_ERROR << "Invalid item: " << itemPath;
                    return;
                }

                for (const auto& [service, ifaces] : serviceIfaces)
                {
                    for (const auto& iface : ifaces)
                    {
                        std::optional<InventoryItemType> type;
                        if (iface ==
                                "xyz.openbmc_project.Inventory.Item.Board" ||
                            iface ==
                                "xyz.openbmc_project.Inventory.Item.Chassis")
                        {
                            type = InventoryItemType::chassis;
                        }

                        if (type)
                        {
                            itemNames.push_back({name, *type});
                        }
                    }
                }
            }

            callback(itemNames);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", 0, interfaces);
}

template <typename F>
void retrieveMappings(
    F& handler, const std::vector<std::pair<std::string, InventoryItemType>>&
                    inventoryItems)
{
    for (const auto& [item, type] : inventoryItems)
    {
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

void addMembers(bmcweb::AsyncResp& asyncResp,
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

        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);
        utils::getInventoryNames(
            [asyncResp](const std::vector<
                        std::pair<std::string, utils::InventoryItemType>>&
                            inventoryItems) {
                const auto handler =
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
                        telemetry::addMembers(*asyncResp, uriToDbus);
                    };
                retrieveMappings(handler, inventoryItems);
            },
            asyncResp);
    }
};

namespace telemetry
{

void addMetricProperty(
    bmcweb::AsyncResp& asyncResp, const std::string& id,
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
        if (params.size() != 1)
        {
            bmcweb::AsyncResp asyncResp(res);
            messages::internalError(asyncResp.res);
            return;
        }

        const std::string& id = params[0];
        auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res, [&res, id] {
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

        utils::getInventoryNames(
            [asyncResp, id](const std::vector<
                            std::pair<std::string, utils::InventoryItemType>>&
                                inventoryItems) {
                const auto handler =
                    [asyncResp, id](const boost::beast::http::status status,
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
                        telemetry::addMetricProperty(*asyncResp, id, uriToDbus);
                    };
                retrieveMappings(handler, inventoryItems);
            },
            asyncResp);
    }
};

} // namespace redfish
