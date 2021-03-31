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

            const std::vector<std::string> nodes = {"CPU", "Memory", "Storage_RW"};

            for (const std::string& dbusObjectName : nodes)
            {
                crow::connections::systemBus->async_method_call(
                    [asyncResp, dbusObjectName](const boost::system::error_code ec,
                                           std::variant<double> value) {
                        if (ec)
                        {
                            asyncResp->res.jsonValue = {
                                {"message", "Error " + ec.message()}};
                        }
                        else
                        {
                            asyncResp->res.jsonValue[dbusObjectName] =
                                std::to_string(std::get<double>(value));
                        }
                    },
                    "xyz.openbmc_project.HealthMon",
                    "/xyz/openbmc_project/sensors/utilization/" + dbusObjectName,
                    "org.freedesktop.DBus.Properties", "Get",
                    "xyz.openbmc_project.Sensor.Value", "Value");
            }
        });
}

} // namespace bmc_health
} // namespace crow