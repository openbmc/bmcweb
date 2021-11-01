
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
inline void populateOneManagerDiagnosticDataEntry(
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

inline void managerDiagnosticDataGetUtilization(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    // Must use "bmc" as a "chassis".
    const std::string chassisId = "bmc";

    // Get a list of all of the sensors that implement Sensor.Value
    // and get the path and service names associated with the sensors.
    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

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

                // There should be exactly 1 service with an object of
                // Sensor.Value for all the utilization sensors that are related
                // to the ManagerDiagnosticData resource.
                const size_t numServices = servicesAndInterfaces.size();
                if (numServices != 1)
                {
                    continue;
                }

                // Fetch the reading and populate the response.
                const std::string& dbusService = servicesAndInterfaces[0].first;
                populateOneManagerDiagnosticDataEntry(aResp, objectPath,
                                                      dbusService);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, interfaces);
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
