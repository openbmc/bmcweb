#pragma once

#include <app.hpp>
#include <async_resp.hpp>
#include <http_request.hpp>
#include <nlohmann/json.hpp>
#include <privileges.hpp>
#include <routing.hpp>

#include <string>

namespace redfish
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

inline void managerDiagnosticDataGetUtilization(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp)
{
    const std::string chassisId = "bmc"; // Must use "bmc" as a "chassis"

    const std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};

    // Get a list of all of the sensors that implement Sensor.Value
    // and get the path and service name associated with the sensor
    // and just get the readings from the CPU_Kernel and CPU_User sensors
    // from the health monitor service.
    crow::connections::systemBus->async_method_call(
        [aResp](const boost::system::error_code ec,
                const GetSubTreeType& subtree) {
        if (ec)
        {
            messages::internalError(aResp->res);
            return;
        }

        // Keep track of values that are known so far. If both requested
        // values become available, write them into the response.
        std::unordered_map<std::string, double> cpuUsagePercentValues;
        std::unordered_map<std::string, double> memoryUsagePercentValues;

        // GetSubTreeType is an a{sa{sas}}
        // Loop through each sa{sas} in the a{sa{sas}}
        for (const auto& [path, servicesAndInterfaces] : subtree)
        {
            sdbusplus::message::object_path objectPath(path);
            std::string name = objectPath.filename();

            std::string processorMetricName;
            if (name == "CPU_Kernel")
            {
                processorMetricName = "KernelPercent";
            }
            else if (name == "CPU_User")
            {
                processorMetricName = "UserPercent";
            }

            std::string memoryMetricName;
            if (name == "Memory_Available")
            {
                memoryMetricName = "AvailablePercent";
            }
            else if (name == "Memory_BufferAndCache")
            {
                memoryMetricName = "BufferAndCachePercent";
            }
            else if (name == "Memory_Free")
            {
                memoryMetricName = "FreePercent";
            }
            else if (name == "Memory_Shared")
            {
                memoryMetricName = "SharedPercent";
            }
            else if (name == "Memory_Used")
            {
                memoryMetricName = "UsedPercent";
            }

            std::string serviceName;
            std::unordered_map<std::string, double>* valuesPtr;
            if (processorMetricName != "")
            {
                serviceName = processorMetricName;
                valuesPtr = &cpuUsagePercentValues;
            }
            else if (memoryMetricName != "")
            {
                serviceName = memoryMetricName;
                valuesPtr = &memoryUsagePercentValues;
            }

            if (serviceName == "")
            {
                continue;
            }

            // interfaceMaps.first (s) is the object path
            // interfaceMaps.second (a{sas}) are the services containing the
            // paths and a full list of the interfaces the paths contain
            const size_t numServices = servicesAndInterfaces.size();

            // Should be exactly 1 service with an object of Sensor.Value
            // type and a name representing BMC's User and Kernel CPU usag
            if (numServices != 1)
            {
                BMCWEB_LOG_DEBUG
                    << "Expecting exactly 1 service for CPU/memory metrics, got "
                    << numServices;
                continue;
            }

            // Had to use synchronous DBus method call due to the difficult
            // in keeping cpuUsagePercentValues alive during async method
            // calls
            const std::string& dbusService = servicesAndInterfaces[0].first;
            sdbusplus::message::message m =
                crow::connections::systemBus->new_method_call(
                    dbusService.c_str(), objectPath.str.c_str(),
                    "org.freedesktop.DBus.Properties", "Get");
            m.append("xyz.openbmc_project.Sensor.Value");
            m.append("Value");

            try
            {
                sdbusplus::message::message reply =
                    crow::connections::systemBus->call(m);
                std::variant<double> value;
                reply.read(value);
                const double* d = std::get_if<double>(&value);
                if (d == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Null value returned for " << path
                                     << " of service " << serviceName;
                }
                else
                {
                    (*valuesPtr)[serviceName] = *d;
                }
            }
            catch (const std::exception& e)
            {
                BMCWEB_LOG_ERROR << "Could not get the value of object "
                                 << objectPath.str << " of service "
                                 << processorMetricName;
            }
        }

        // Populate response only if both kernel and user percentages are
        // populated
        if (cpuUsagePercentValues.size() == 2)
        {
            for (const auto& kv : cpuUsagePercentValues)
            {
                aResp->res.jsonValue["CPUUtilization"]["#ProcessorMetrics"]
                                    [kv.first] = kv.second;
            }
        }
        else
        {
            if (cpuUsagePercentValues.count("KernelPercent") == 0)
            {
                BMCWEB_LOG_DEBUG << "Could not find kernel space CPU "
                                    "usage, not populating CPUUtilization";
            }
            if (cpuUsagePercentValues.count("UserPercent") == 0)
            {
                BMCWEB_LOG_DEBUG << "Could not find userspace CPU usage, "
                                    "not populating CPUUtilization";
            }
            messages::internalError(aResp->res);
        }

        // Populate memory utilization
        for (const auto& [key, value] : memoryUsagePercentValues)
        {
            aResp->res.jsonValue["MemoryStat"]["#MemoryStat"][key] = value;
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
        .privileges(redfish::privileges::getManagerDiagnosticData)
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request& /*req*/,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.jsonValue["@odata.type"] =
            "#ManagerDiagnosticData.ManagerDiagnosticData";
        asyncResp->res.jsonValue["@odata.id"] =
            "/redfish/v1/Managers/bmc/ManagerDiagnosticData";

        // Gets both CPU and memory usage
        managerDiagnosticDataGetUtilization(asyncResp);
        });
}

} // namespace redfish
