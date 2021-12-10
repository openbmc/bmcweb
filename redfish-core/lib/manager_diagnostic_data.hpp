
#pragma once
#include <string>
#include <string_view>

namespace redfish
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using SensorVariant =
    std::variant<int64_t, double, uint32_t, bool, std::string>;

// Populate one field in the ManagerDiagnosticData using the reading of one
// utilization sensor.
inline void populateManagerDiagnosticDataWithUtilizationSensor(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const sdbusplus::message::object_path& dbusObjectPath,
    const std::string& dbusServiceName)
{

    crow::connections::systemBus->async_method_call(
        [aResp, dbusServiceName, dbusObjectPath](
            const boost::system::error_code ec, SensorVariant sensorVariant) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error reading from service "
                                 << dbusServiceName << ", path "
                                 << dbusObjectPath.str << ": " << ec;
                return;
            }
            double value = std::get<double>(sensorVariant);

            // filename() is the substirng after the last separator. Example:
            // dbusObjectPath.str =
            // "/xyz/openbmc_project/sensors/utilization/CPU" utilSensorName =
            // "CPU"
            std::string utilSensorName = dbusObjectPath.filename();

            // Kernel CPU utilization
            // DBus utilization sensor name: "CPU_Kernel"
            // Redfish property:
            // "ManagerDiagnosticData.ProcessorStatistics.KernelPercent"
            if (utilSensorName == "CPU_Kernel")
            {
                aResp->res.jsonValue["ProcessorStatistics"]["KernelPercent"] =
                    value;
            }

            // Userspace CPU utilization
            // DBus utilization sensor name: "CPU_User"
            // Redfish property:
            // "ManagerDiagnosticData.ProcessorStatistics.UserPercent"
            if (utilSensorName == "CPU_User")
            {
                aResp->res.jsonValue["ProcessorStatistics"]["UserPercent"] =
                    value;
            }
        },
        dbusServiceName.c_str(), dbusObjectPath.str.c_str(),
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Sensor.Value", "Value");
}

inline int extractI2CIdFromPath(std::string_view path) {
    int result {};
    size_t idx = path.rfind("i2c_");
    if (idx == std::string_view::npos) {
        return -1;
    }
    auto [ptr, ec] { std::from_chars(path.data()+idx+4, path.data()+path.size(), result) };
    if (ec == std::errc()) {
        return result;
    }
    return -1;
}

// Populate one I2CBusStatistics using one I2CStats object.
inline void populateManagerDiagnosticDataWithI2CStats(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const sdbusplus::message::object_path& dbusObjectPath,
    const std::string& dbusServiceName)
{
    const int i2cId = extractI2CIdFromPath(dbusObjectPath.str);
    if (i2cId == -1) return;

    crow::connections::systemBus->async_method_call(
        [aResp, dbusServiceName, dbusObjectPath, i2cId](
            const boost::system::error_code ec,
            std::vector<std::pair<std::string, SensorVariant>> properties) {

            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error reading from service "
                                 << dbusServiceName << ", path "
                                 << dbusObjectPath.str << ": " << ec;
                return;
            }

            for (const auto& [name, var] : properties) {
                bool is_double = std::visit([](auto&& arg) -> bool {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_arithmetic_v<T>) {
                        return true;
                    }
                    return false;
                }, var);
                if (!is_double) continue;

                if (aResp->res.jsonValue.find("I2CBuses") == aResp->res.jsonValue.end())
                {
                    aResp->res.jsonValue["I2CBuses"] = nlohmann::json::array();
                }
                nlohmann::json& i2cBuses = aResp->res.jsonValue;
                
                double value = std::get<double>(var);
                nlohmann::json j;
                j["I2CBusName"] = "i2c-" + std::to_string(i2cId);
                if (name == "BusErrorCount") {
                    j["BusErrorCount"] = value;
                } else if (name == "NACKCount") {
                    j["NACKCount"] = value;
                } else if (name == "TotalTransactionCount") {
                    j["TotalTransactionCount"] = value;
                }
                i2cBuses.push_back(j);
            }
        },
        dbusServiceName.c_str(), dbusObjectPath.str.c_str(),
        "org.freedesktop.DBus.Properties", "GetAll",
        "xyz.openbmc_project.I2CStats");
}

inline void managerDiagnosticDataGetUtilization(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    // Must use "bmc" as a "chassis".
    const std::string chassisId = "bmc";

    // We look for two interfaces:
    // 1. Utilization sensors that implement the Sensor.Value interface
    // 2. I2C stats dbus objects implementing the I2C.I2CStats interface
    const std::array<const char*, 2> interfaces = {
        "xyz.openbmc_project.Sensor.Value",
        "xyz.openbmc_project.I2C.I2CStats"};

    // All sensors on the system may be returned from this call.
    // We need just the readings from the health monitor service.
    // This processing is done by looping over subtree.
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const GetSubTreeType& subtree) {
            if (ec)
            {
                messages::internalError(aResp->res);
                return;
            }

            // GetSubTreeType is an       a{sa{sas}}
            // servicesAndInterfaces is an  sa{sas}
            for (const auto& [path, servicesAndInterfaces] : subtree)
            {
                sdbusplus::message::object_path objectPath(path);
                std::string name = objectPath.filename();

                // There should be exactly 1 service for both utilization sensors
                // and I2C stats objects
                const size_t numServices = servicesAndInterfaces.size();
                if (numServices != 1)
                {
                    continue;
                }

                // The as in {sas} is a list of interfaces. If it contains
                // "xyz.openbmc_project.I2C.I2CStats" we treat it as an I2C
                // statistics DBus object. Otherwise we treat it as a utility sensor.
                bool is_i2c = false;
                for (const std::string& iface : servicesAndInterfaces[1].second) {
                    if (iface == "xyz.openbmc_project.I2C.I2CStats") {
                        is_i2c = true;
                    }
                }

                // Fetch the reading and populate the response.
                const std::string& dbusService = servicesAndInterfaces[0].first;
                if (is_i2c) {
                    populateManagerDiagnosticDataWithI2CStats(aResp,
                        objectPath, dbusService);
                } else {
                    populateManagerDiagnosticDataWithUtilizationSensor(aResp,
                        objectPath, dbusService);
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project", 2, interfaces);
}
inline void requestRoutesManagerDiagnosticData(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/Managers/bmc/ManagerDiagnosticData")
        .privileges(redfish::privileges::getManagerCollection)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ManagerDiagnosticData.v1_0_0.ManagerDiagnosticData";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/Managers/bmc/ManagerDiagnosticData";
                const std::string& idAndName = "BMC's ManagerDiagnosticData";
                asyncResp->res.jsonValue["Id"] = idAndName;
                asyncResp->res.jsonValue["Name"] = idAndName;

                // Gets both CPU and memory usage
                managerDiagnosticDataGetUtilization(asyncResp);
            });
}

} // namespace redfish
