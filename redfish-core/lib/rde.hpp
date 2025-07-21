#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"

#include <boost/beast/http/status.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <filesystem>
#include <format>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace redfish
{

constexpr std::string_view rdeRoot = "/redfish/v1/Systems/system";
constexpr std::string_view pldmService = "xyz.openbmc_project.PLDM";
std::map<uint8_t, std::string> eidUUIDMap;

std::vector<std::string> uuidMapP0;
std::vector<std::string> uuidMapP1;

using SchemaResourcesType =
    std::map<std::string,
             std::map<std::string, dbus::utility::DbusVariantType>>;
using RDEVariantType = std::variant<std::vector<std::string>, std::string,
                                    uint8_t, SchemaResourcesType>;
using RDEPropertiesMap = std::vector<std::pair<std::string, RDEVariantType>>;
using ObjectPath = sdbusplus::message::object_path;

std::vector<struct Resource> resourceVect;
std::map<std::string, std::string> nameUriNameMap = {
    {"Processors.Processors", "Processors"}};

struct Resource
{
    std::string rid;
    std::string subUri;
    uint8_t eid;
    std::string uuid;
    std::string schemaName;
    std::string schemaVersion;
    int64_t schemaClass;
    std::string fullUri;
};

// GET handler
inline void rdeGetHandler(App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("RDE : Get handler.");
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // TODO: Implement the GET request handling logic here.
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.jsonValue["status"] = "success";
}

// PATCH handler
inline void rdePatchHandler(App& app, const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    BMCWEB_LOG_DEBUG("RDE : Patch handler.");

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }

    // TODO: Implement the PATCH request handling logic here.
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.jsonValue["status"] = "success";
}

std::string updateDeviceID(const std::string& path, const std::string& deviceId)
{
    // Find the position of the second slash
    size_t firstSlash = path.find('/');
    size_t secondSlash = path.find('/', firstSlash + 1);

    if (secondSlash == std::string::npos)
    {
        // Path has only one segment or is malformed
        return path;
    }

    // Insert the string after the first segment
    std::string modifiedPath = path;
    modifiedPath.insert(secondSlash, "/" + deviceId);
    return modifiedPath;
}

inline void updateDeviceName(std::string& inputUri)
{
    for (const auto& [key, val] : nameUriNameMap)
    {
        size_t pos = inputUri.find(key);
        if (pos != std::string::npos)
        {
            inputUri.replace(pos, key.length(), val);
            break;
        }
    }
}

inline void rdeHandler(const crow::Request& /*req*/,
                       const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/)
{
    // TODO Implement rdeHandler
}

inline std::string getDeviceID(std::string uuid)
{
    if (std::find(uuidMapP0.begin(), uuidMapP0.end(), uuid) != uuidMapP0.end())
    {
        return "P0";
    }
    if (std::find(uuidMapP1.begin(), uuidMapP1.end(), uuid) != uuidMapP1.end())
    {
        return "P1";
    }
    return "";
}

inline void
    getRDEResourceData(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& expDeviceID,
                       const std::string& objectPath, bool isRDERoot)
{
    sdbusplus::asio::getAllProperties(*crow::connections::systemBus,
                                      std::string(pldmService), objectPath,
                                      "xyz.openbmc_project.RDE.Device",
                                      [req, asyncResp, expDeviceID, isRDERoot](
                                          const boost::system::error_code& ec,
                                          const RDEPropertiesMap& properties) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("DBUS response error: {}", ec.message());
            messages::internalError(asyncResp->res);
            return;
        }

        const std::string* uuid = nullptr;
        const uint8_t* eid = nullptr;
        SchemaResourcesType schemaResources;

        const bool success = sdbusplus::unpackPropertiesNoThrow(
            dbus_utils::UnpackErrorPrinter(), properties, "DeviceUUID", uuid,
            "EID", eid, "SchemaResources", schemaResources);

        if (!success)
        {
            BMCWEB_LOG_ERROR(
                "getRDEResourceData: unpackPropertiesNoThrow  error");
            messages::internalError(asyncResp->res);
            return;
        }

        if (uuid == nullptr)
        {
            BMCWEB_LOG_ERROR("getRDEResourceData: Failed to get DeviceUUID");
            messages::propertyNotUpdated(asyncResp->res, "DeviceUUID");
        }

        if (eid == nullptr)
        {
            BMCWEB_LOG_ERROR("getRDEResourceData: Failed to get EID");
            messages::propertyNotUpdated(asyncResp->res, "EID");
        }

        std::string deviceID = getDeviceID(*uuid);
        if (expDeviceID != deviceID)
        {
            BMCWEB_LOG_DEBUG(
                "getRDEResourceData: DeviceID not matched, expDeviceID: {} deviceID: {}",
                expDeviceID, deviceID);
            return;
        }
        nlohmann::json resourceArray = nlohmann::json::array();

        for (const auto& [rid, valueMap] : schemaResources)
        {
            std::string fullUri;
            struct Resource res;
            res.rid = rid;

            // get subURI
            auto subUriIt = valueMap.find("subUri");
            if (subUriIt != valueMap.end())
            {
                auto* subUri = std::get_if<std::string>(&subUriIt->second);
                if (subUri)
                {
                    std::string uri = *subUri;
                    updateDeviceName(uri);
                    res.subUri = uri;
                    if (!deviceID.empty())
                    {
                        uri = updateDeviceID(uri, deviceID);
                    }
                    else
                    {
                        BMCWEB_LOG_INFO(
                            "deviceID is empty, going with default");
                    }
                    fullUri = std::string(rdeRoot) + uri;
                }
            }

            nlohmann::json resourceEntry = {{"@odata.id", fullUri}};

            res.fullUri = fullUri;
            res.eid = *eid;
            res.uuid = *uuid;

            auto nameIt = valueMap.find("schemaName");
            if (nameIt != valueMap.end())
            {
                const auto* name = std::get_if<std::string>(&nameIt->second);
                resourceEntry["MajorSchemaName"] = *name;
                res.schemaName = *name;
            }
            auto classItr = valueMap.find("schemaClass");
            if (classItr != valueMap.end())
            {
                const auto* schemaClass =
                    std::get_if<int64_t>(&classItr->second);
                resourceEntry["SchemaClass"] = *schemaClass;
                res.schemaClass = *schemaClass;
            }
            auto verIt = valueMap.find("schemaVersion");
            if (verIt != valueMap.end())
            {
                const auto* schemaVersion =
                    std::get_if<std::string>(&verIt->second);
                resourceEntry["MajorSchemaVersion"] = *schemaVersion;
                res.schemaVersion = *schemaVersion;
            }
            resourceArray.push_back(resourceEntry);
            resourceVect.push_back(res);
        }

        if (isRDERoot)
        {
            asyncResp->res.jsonValue["Resources"] = resourceArray;
        }
        else
        {
            rdeHandler(req, asyncResp);
        }
    });
}

inline bool loadRDEDeviceMetadata()
{
    constexpr const char* rdeDeviceMetadataFile =
        "/etc/pldm/rde_device_metadata.json";

    nlohmann::json jsonData = nlohmann::json::object();

    if (!std::filesystem::exists(rdeDeviceMetadataFile))
    {
        BMCWEB_LOG_ERROR("RDE: Device metadata file not found: {}",
                         rdeDeviceMetadataFile);
        return false;
    }

    std::ifstream file(rdeDeviceMetadataFile);
    if (!file.is_open())
    {
        BMCWEB_LOG_ERROR("RDE: Failed to open device metadata file: {}",
                         rdeDeviceMetadataFile);
        return false;
    }

    try
    {
        if (file.peek() == std::ifstream::traits_type::eof())
        {
            BMCWEB_LOG_ERROR("RDE: Device metadata file is empty: {}",
                             rdeDeviceMetadataFile);
            return false;
        }

        file >> jsonData;

        if (!jsonData.is_object())
        {
            BMCWEB_LOG_ERROR(
                "RDE: Device metadata file does not contain a valid JSON objects.");
            return false;
        }

        const std::string deviceKey = "Processors";
        if (!jsonData.contains(deviceKey) || !jsonData[deviceKey].is_object())
        {
            BMCWEB_LOG_ERROR("RDE: Missing or invalid '{}' section in metadata",
                             deviceKey);
            return false;
        }

        const auto& processors = jsonData[deviceKey];
        std::vector<std::string> processorKeys = {"P0", "P1"};

        for (const auto& procKey : processorKeys)
        {
            if (!processors.contains(procKey))
            {
                BMCWEB_LOG_ERROR("RDE: Missing processor key '{}'", procKey);
                continue;
            }

            const auto& proc = processors.at(procKey);
            if (!proc.contains("UUIDs") || !proc["UUIDs"].is_array())
            {
                BMCWEB_LOG_ERROR("RDE: Missing or invalid 'UUIDs' for '{}'",
                                 procKey);
                continue;
            }

            for (const auto& uuid : proc["UUIDs"])
            {
                if (!uuid.is_string())
                {
                    BMCWEB_LOG_ERROR("RDE: Invalid UUID format in '{}'",
                                     procKey);
                    continue;
                }

                const std::string uuidStr = uuid.get<std::string>();
                if (procKey == "P0")
                {
                    uuidMapP0.push_back(uuidStr);
                }
                else if (procKey == "P1")
                {
                    uuidMapP1.push_back(uuidStr);
                }
            }
        }
    }
    catch (const nlohmann::json::parse_error& e)
    {
        BMCWEB_LOG_ERROR("RDE: JSON parse error: {}", e.what());
        return false;
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_ERROR(
            "RDE: Unexpected error while reading device metadata file: {}",
            e.what());
        return false;
    }
    return true;
}

inline void reset()
{
    eidUUIDMap.clear();
    resourceVect.clear();
}

inline void
    getProcRdeDevices(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& deviceId, bool isRDERoot)
{
    reset();
    constexpr std::array<std::string_view, 1> interfaces = {
        "xyz.openbmc_project.RDE.Device"};
    dbus::utility::getSubTreePaths(
        "/", 0, interfaces,
        [req, asyncResp, deviceId,
         isRDERoot](const boost::system::error_code& ec,
                    const dbus::utility::MapperGetSubTreePathsResponse&
                        objPaths) mutable {
        if (ec)
        {
            BMCWEB_LOG_ERROR(
                "getProcRdeDevices : getSubTreePaths DBUS error: {}", ec);
            messages::internalError(asyncResp->res);
            return;
        }

        for (const std::string& path : objPaths)
        {
            sdbusplus::message::object_path objPath(path);
            std::string pathName = objPath.filename();
            if (pathName.empty())
            {
                BMCWEB_LOG_ERROR("Failed to find '/' in {}", path);
                continue;
            }
            BMCWEB_LOG_DEBUG("Found device with ObjectPath {}", path);
            getRDEResourceData(req, asyncResp, deviceId, path, isRDERoot);
        }
    });
}

inline void handleProcRDE(crow::App& app, const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& systemName,
                          const std::string& deviceId,
                          const std::string& schemaPath)
{
    BMCWEB_LOG_DEBUG("RDE Request Handling[{} {} {}]", systemName, deviceId,
                     schemaPath);

    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    // TODO implement handleProcRDE process
}

inline void handleProcessorRoot(
    [[maybe_unused]] crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    [[maybe_unused]] const std::string& systemName, const std::string& deviceId)
{
    asyncResp->res.jsonValue["@odata.type"] = "#RDE.v1_0_0.ProcessorRoot";
    asyncResp->res.jsonValue["@odata.id"] = std::string(req.url().buffer());
    asyncResp->res.jsonValue["Name"] = "Redfish Dynamic Extensions";

    if (!loadRDEDeviceMetadata())
    {
        BMCWEB_LOG_ERROR("Invalid or missing device identifiers in metadata");
        asyncResp->res.result(boost::beast::http::status::not_acceptable);
        asyncResp->res.jsonValue = {
            {"error", "Invalid or missing device identifier"},
            {"status", "failed"}};
        return;
    }

    getProcRdeDevices(req, asyncResp, deviceId, true);
}

inline void requestRoutesRDEService(crow::App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/Oem/AMD/SocConfiguration/<path>")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::head, boost::beast::http::verb::get,
                 boost::beast::http::verb::patch)(
            std::bind_front(handleProcRDE, std::ref(app)));

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/Oem/AMD/SocConfiguration")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            std::bind_front(handleProcessorRoot, std::ref(app)));
}

} // namespace redfish
