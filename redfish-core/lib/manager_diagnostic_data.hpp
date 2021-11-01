
#pragma once
#include <boost/algorithm/string/predicate.hpp>
#include <sdbusplus/asio/property.hpp>

#include <string>
#include <string_view>

namespace redfish
{

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

using GetObjectType =
    std::vector<std::pair<std::string, std::vector<std::string>>>;

using SensorVariantType =
    std::variant<int64_t, double, uint32_t, bool, std::string>;

using EndpointVariantType = std::variant<std::vector<std::string>>;

using EndpointType = std::vector<std::string>;

// Step 1
inline void
    findBmcInventory(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

// Step 2
inline void
    findBmcAssociation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& bmcObjectPath);

// Step 3
inline void findBmcAssociationEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& bmcAssociationObjectPath,
    const std::string& bmcAssociationService);

// Step 4
inline void populateOneManagerDiagnosticDataEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& diagnosticDataObjectPath);

std::optional<std::pair<std::string, std::string>>
    checkAndGetSoleGetSubTreeResponse(const GetSubTreeType& subtree)
{

    std::optional<std::pair<std::string, std::string>> ret;

    if (subtree.empty())
    {
        BMCWEB_LOG_DEBUG << "Can't find bmc D-Bus object!";
        return {};
    }

    if (subtree.size() > 1)
    {
        BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
        return {};
    }

    // Assume only 1 bmc D-Bus object
    // Throw an error if there is more than 1
    if (subtree.size() > 1)
    {
        BMCWEB_LOG_DEBUG << "Found more than 1 bmc D-Bus object!";
        return {};
    }

    if (subtree[0].first.empty() || subtree[0].second.size() != 1)
    {
        BMCWEB_LOG_DEBUG << "Error getting bmc D-Bus object!";
        return {};
    }

    return std::make_pair(subtree[0].first, subtree[0].second[0].first);
}

// Returns the sole non-Object Mapper service
std::optional<std::string>
    checkAndGetSoleNonMapperGetObjectResponse(const GetObjectType& objects)
{

    std::optional<std::pair<std::string, std::string>> ret;

    if (objects.empty())
    {
        return {};
    }

    std::optional<std::string> service;
    for (const std::pair<std::string, std::vector<std::string>>&
             serviceAndIfaces : objects)
    {
        const std::string& s = serviceAndIfaces.first;
        if (s != "xyz.openbmc_project.ObjectMapper")
        {
            if (service.has_value())
            { // Already found one
                return {};
            }
            service = s;
        }
    }

    return service;
}

// Step 1 for finding BMC diagnostic data:
// Find BMC Inventory item in the system.
inline void
    findBmcInventory(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus response error on GetSubTree " << ec;
                return;
            }

            // For Entity-Manager based systems ("dynamic inventory stack"):
            // subtree[0].first           may be
            // "/xyz/openbmc_project/inventory/system/bmc/bmc".
            // subtree[0].second[0].first may be {
            // "xyz.openbmc_project.EntityManager",
            //   {"xyz.openbmc_project.AddObject"
            //   "xyz.openbmc_project.Inventory.Item.Bmc"} }.

            // For Inventory-Manager based systems ("static inventory stack"):
            // subtree[0].first           may be
            // "/xyz/openbmc_project/inventory/bmc", depending on compile-time
            // configuration subtree[0].second[0].first may be {
            // ""xyz.openbmc_project.Inventory.Manager",
            //   {"org.freedesktop.DBus.Introspectable",
            //    "org.freedesktop.DBus.Peer",
            //    "org.freedesktop.DBus.Properties",
            //    "xyz.openbmc_project.Inventory.Item.Bmc"} }
            std::optional<std::pair<std::string, std::string>> pathAndService =
                checkAndGetSoleGetSubTreeResponse(subtree);
            if (!pathAndService.has_value())
            {
                return;
            }

            // Found BMC object path. Ask ObjectMapper about the reverse
            // Association edges.
            findBmcAssociation(asyncResp, pathAndService.value().first);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/inventory", int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Inventory.Item.Bmc"});
}

// Step 2 for finding BMC diagnostic data:
// Given the BMC inventory's path, find the Associations related to it.
inline void
    findBmcAssociation(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& bmcObjectPath)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, bmcObjectPath](const boost::system::error_code ec,
                                   const GetSubTreeType& subtree) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBus error getting associations for BMC "
                                    "inventory object "
                                 << ec;
                return;
            }

            // The expected values may be as follows, depending on whether the
            // static/dynamic stack is used. subtree[0].first:
            // "/xyz/openbmc_project/inventory/system/bmc/bmc/bmc_diagnostic_data"
            // subtree[0].second[0]: "xyz.openbmc_project.ObjectMapper"
            std::optional<std::pair<std::string, std::string>> pathAndService =
                checkAndGetSoleGetSubTreeResponse(subtree);
            if (!pathAndService.has_value())
            {
                return;
            }

            findBmcAssociationEndpoints(asyncResp, pathAndService.value().first,
                                        pathAndService.value().second);
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", bmcObjectPath,
        int32_t(0),
        std::array<const char*, 1>{"xyz.openbmc_project.Association"});
}

// Step 3: List all Association endpoints.
inline void findBmcAssociationEndpoints(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& bmcAssociationObjectPath,
    const std::string& bmcAssociationService)
{

    printf("findBmcAssociationEndpoints %s %s\n",
           bmcAssociationObjectPath.c_str(), bmcAssociationService.c_str());

    // No need for variant
    sdbusplus::asio::getProperty<EndpointType>(
        *crow::connections::systemBus, bmcAssociationService,
        bmcAssociationObjectPath, "xyz.openbmc_project.Association",
        "endpoints",
        [asyncResp](const boost::system::error_code ec, const EndpointType& v) {
            if (ec)
            {
                printf("DBus error getting association endpoints\n");
                return;
            }

            for (const std::string& endpoint : v)
            {
                printf("Endpoint is %s\n", endpoint.c_str());
                populateOneManagerDiagnosticDataEntry(asyncResp, endpoint);
            }
        });
}

// Step 4: For each endpoint, find their corresponding service name. Then get
// the readings.
inline void populateOneManagerDiagnosticDataEntry(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& diagnosticDataObjectPath)
{

    crow::connections::systemBus->async_method_call(
        [asyncResp, diagnosticDataObjectPath](
            const boost::system::error_code ec, const GetObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Error occurred GetObject";
                return;
            }

            std::optional<std::string> serviceName =
                checkAndGetSoleNonMapperGetObjectResponse(objects);
            if (!serviceName.has_value())
            {
                BMCWEB_LOG_ERROR << "Cannot get service from object name"
                                 << diagnosticDataObjectPath;
                return;
            }

            // I2C statistics implement the "" interfaces
            bool isI2CStats = false;

            // By this point, subtree[0].second should have only 1 entry,
            // meaning only 1 service is serving the sensor with this
            // diagnosticDataObjectPath
            if (serviceName.value().find("i2cstats") != std::string::npos)
            {
                isI2CStats = true;
            }

            if (!isI2CStats)
            {
                sdbusplus::asio::getProperty<double>(
                    *crow::connections::systemBus, serviceName.value(),
                    diagnosticDataObjectPath,
                    "xyz.openbmc_project.Sensor.Value", "Value",
                    [asyncResp, diagnosticDataObjectPath](
                        const boost::system::error_code ec, const double v) {
                        if (ec)
                        {
                            BMCWEB_LOG_DEBUG
                                << "DBus response error on getProperty " << ec;
                            return;
                        }
                        if (boost::ends_with(diagnosticDataObjectPath,
                                             "CPU_Kernel"))
                        {
                            asyncResp->res.jsonValue["ProcessorStatistics"]
                                                    ["KernelPercent"] = v;
                        }
                        else if (boost::ends_with(diagnosticDataObjectPath,
                                                  "CPU_User"))
                        {
                            asyncResp->res.jsonValue["ProcessorStatistics"]
                                                    ["UserPercent"] = v;
                        }
                    });
            }
            else
            {
                // I2C statistics - to be added later
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        diagnosticDataObjectPath, std::array<const char*, 0>{});
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

                // Start the first step of the 4-step process to populate
                // the BMC diagnostic data
                findBmcInventory(asyncResp);
            });
}

} // namespace redfish
