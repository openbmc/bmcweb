namespace crow
{
namespace bmc_health
{

inline void requestRoutes(App& app)
{
    BMCWEB_ROUTE(app, "/bmchealth/")
        .methods(boost::beast::http::verb::get)([](const crow::Request&,
                                                   crow::Response& res) {
            auto asyncResp = std::make_shared<bmcweb::AsyncResp>(res);

            const std::string nodes[] = {"CPU", "Memory", "Storage_RW"};

            for (int i = 0; i < 3; i++)
            {
                const std::string& node_name = nodes[i];
                crow::connections::systemBus->async_method_call(
                    [asyncResp, node_name](const boost::system::error_code ec,
                                           std::variant<double> value) {
                        if (ec)
                        {
                            asyncResp->res.jsonValue = {
                                {"message", "Error " + ec.message()}};
                        }
                        else
                        {
                            asyncResp->res.jsonValue[node_name] =
                                std::to_string(std::get<double>(value));
                        }
                    },
                    "xyz.openbmc_project.HealthMon",
                    "/xyz/openbmc_project/sensors/utilization/" + node_name,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Sensor.Value", "Value");
            }
        });
}

} // namespace bmc_health
} // namespace crow