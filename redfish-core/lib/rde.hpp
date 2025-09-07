#pragma once

#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "task.hpp"

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
struct RDESchemaEntry
{
    std::string rid;
    std::string subUri;
    std::string schemaName;
    int64_t schemaClass;
    std::string schemaVersion;
    std::string fullUri;
};
using RDESchemaResource = std::vector<RDESchemaEntry>;
using SchemaResourcesType =
    std::map<std::string,
             std::map<std::string, dbus::utility::DbusVariantType>>;
using RDEVariantType = std::variant<std::vector<std::string>, std::string,
                                    uint8_t, SchemaResourcesType>;
using RDEPropertiesMap = std::vector<std::pair<std::string, RDEVariantType>>;
using ObjectPath = sdbusplus::message::object_path;
using DbusVariantType = std::variant<std::string, uint16_t, int64_t, bool>;

constexpr const char* procSchemaName = "Processors";
constexpr const char* networkSchemaName = "NetworkAdapter";
constexpr const char* rdeDeviceInterface = "xyz.openbmc_project.RDE.Device";
constexpr const char* pldmService = "xyz.openbmc_project.PLDM";
constexpr const char* rdeSignalInterface =
    "xyz.openbmc_project.RDE.OperationTask";
constexpr const char* rdeManagerPath = "/xyz/openbmc_project/RDE/Manager";
constexpr const char* rdeManagerInterface = "xyz.openbmc_project.RDE.Manager";
constexpr const char* rdeOperationMethod = "StartRedfishOperation";
constexpr const char* rdeOperationTypeRead =
    "xyz.openbmc_project.RDE.Common.OperationType.READ";
constexpr const char* rdeOperationTypePatch =
    "xyz.openbmc_project.RDE.Common.OperationType.UPDATE";
constexpr const char* rdePayloadFormatInline =
    "xyz.openbmc_project.RDE.Manager.PayloadFormatType.Inline";
constexpr const char* rdeEncodingFormatJSON =
    "xyz.openbmc_project.RDE.Manager.EncodingFormatType.JSON";
constexpr const char* rdeSignalMember = "TaskUpdated";
constexpr uint16_t completionCodeOperationCompleted = 7;
constexpr uint16_t completionCodeOperationFailed = 8;
const std::string operationIdFile = "/var/run/rde-operation-id";
constexpr int rdeTaskTimeoutMinutes = 5;

/** @brief Match rule template for RDE Operation Task signal */
inline constexpr std::string_view rdeOpTaskMatchTemplate =
    "type='signal',sender='{}',interface='{}',member='{}',path='{}'";

/** @brief Build match rule for RDE Operation Task using object path */
inline std::string rdeOpTaskMatch(const std::string& objPath)
{
    return std::format(rdeOpTaskMatchTemplate, pldmService, rdeSignalInterface,
                       rdeSignalMember, objPath);
}

/**
 * @brief Resolves deferred bindings in a BEJ-encoded JSON string using a
 * resource registry.
 *
 * This function parses a BEJ JSON string and replaces placeholders like
 * `%L<id>` and `%I<id>` with actual URIs or integer values based on the
 * provided resource registry (`resourceData`). It handles three types of
 * replacements:
 *   1. Bare objects like {"%L123"} → {"@odata.id": "<URI>"}.
 *   2. Inline references like %L123 → "<URI>".
 *   3. Integer references like %I123 → "123".
 *
 * @param[in] bejJsonInput The input BEJ JSON string containing placeholders.
 * @param[in] uriMapJson   The JSON object containing the URI map, where keys
 * arestringified integers (resource IDs) and values are full URIstrings.
 *
 * @return nlohmann::json Parsed JSON object with all placeholders resolved.
 */
inline nlohmann::json handleDeferredBindings(const std::string& bejJsonInput,
                                             const nlohmann::json& uriMapJson)
{
    BMCWEB_LOG_DEBUG("RDE:handleDeferredBindings Enter");

    // Parse resource_registry.txt into uriMap
    std::unordered_map<int, std::string> uriMap;
    for (auto it = uriMapJson.begin(); it != uriMapJson.end(); ++it)
    {
        try
        {
            int resourceId = std::stoi(it.key());
            uriMap[resourceId] = it.value();
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_WARNING(
                " RDE: Invalid resource ID in uriMapJson: Key: {}  error: {}",
                it.key(), e.what());
        }
    }

    std::string bejJson = bejJsonInput;
    std::regex bareObjectRegex("\\{\\s*\"%L(\\d+)\"\\s*\\}");
    std::ostringstream oss1;
    std::sregex_iterator begin1(bejJson.begin(), bejJson.end(),
                                bareObjectRegex),
        end1;
    size_t lastPos1 = 0;

    for (auto it = begin1; it != end1; ++it)
    {
        oss1 << bejJson.substr(
            lastPos1,
            static_cast<size_t>(static_cast<std::ptrdiff_t>(it->position()) -
                                static_cast<std::ptrdiff_t>(lastPos1)));

        int id = std::stoi((*it)[1]);
        auto uriIt = uriMap.find(id);
        if (uriIt != uriMap.end())
        {
            oss1 << R"({"@odata.id":")" << uriIt->second << R"("})";
        }
        else
        {
            oss1 << it->str(); // leave unchanged
        }
        lastPos1 = static_cast<size_t>(it->position() + it->length());
    }
    oss1 << bejJson.substr(lastPos1);
    bejJson = oss1.str();

    std::regex lPattern(R"(%L(\d+))");
    std::ostringstream oss2;
    std::sregex_iterator begin2(bejJson.begin(), bejJson.end(), lPattern), end2;
    size_t lastPos2 = 0;
    for (auto it = begin2; it != end2; ++it)
    {
        oss2 << bejJson.substr(
            lastPos2,
            static_cast<size_t>(static_cast<std::ptrdiff_t>(it->position()) -
                                static_cast<std::ptrdiff_t>(lastPos2)));

        int id = std::stoi((*it)[1]);
        auto uriIt = uriMap.find(id);
        if (uriIt != uriMap.end())
        {
            oss2 << uriIt->second;
        }
        else
        {
            oss2 << it->str(); // leave unchanged
        }
        lastPos2 = static_cast<size_t>(it->position() + it->length());
    }
    oss2 << bejJson.substr(lastPos2);
    bejJson = oss2.str();

    bejJson = std::regex_replace(bejJson, std::regex(R"(%I(\d+))"), "$1");
    BMCWEB_LOG_DEBUG("RDE: BEJ JSON String Output: {}", bejJson);

    return nlohmann::json::parse(bejJson);
}
/**
 * @brief Process the TaskUpdated signal for an RDE Operation Task.
 *
 * This function handles the D-Bus signal emitted when an RDE task is updated.
 * It extracts the payload and return code from the signal, validates the task
 * context, and updates the task state and messages accordingly.
 *
 * @param ec2       Error code from the signal match.
 * @param msg       D-Bus message containing updated properties.
 * @param taskData  Shared pointer to the task data structure.
 * @return true     Indicates the task is completed.
 */
inline bool
    rdeProcessTaskUpdateSignal(const boost::system::error_code& ec2,
                               sdbusplus::message_t& msg,
                               const std::shared_ptr<task::TaskData>& taskData)
{
    if (ec2)
    {
        BMCWEB_LOG_ERROR("RDE: Signal error - {}", ec2.message());
        taskData->messages.emplace_back(messages::internalError());
        taskData->state = "Exception";
        return task::completed;
    }

    dbus::utility::DBusPropertiesMap changedProperties;
    msg.read(changedProperties);

    const std::string* data = nullptr;
    const uint16_t* rc = nullptr;

    for (const auto& propertyPair : changedProperties)
    {
        if (propertyPair.first == "payload")
        {
            data = std::get_if<std::string>(&propertyPair.second);
        }
        else if (propertyPair.first == "return_code")
        {
            rc = std::get_if<uint16_t>(&propertyPair.second);
        }
    }

    if (data == nullptr || rc == nullptr)
    {
        BMCWEB_LOG_ERROR("RDE: Missing 'payload' or 'return_code' in signal");
        taskData->messages.emplace_back(messages::internalError());
        taskData->state = "Exception";
        return task::completed;
    }

    const auto& jsonBody = taskData->payload->jsonBody;
    if (!jsonBody.contains("uriMap") || !jsonBody["uriMap"].is_object())
    {
        BMCWEB_LOG_ERROR("RDE: No uriMap found in task payload");
        taskData->messages.emplace_back(messages::internalError());
        taskData->state = "Exception";
        return task::completed;
    }
    const auto& uriMapJson = jsonBody["uriMap"];

    nlohmann::json taskEntry;
    try
    {
        const auto& updatedData = handleDeferredBindings(*data, uriMapJson);
        taskEntry["Payload"] = updatedData;
        taskEntry["ReturnCode"] = *rc;
    }
    catch (const std::exception& e)
    {
        BMCWEB_LOG_ERROR("RDE: Exception in payload processing: {}", e.what());
        taskData->messages.emplace_back(messages::internalError());
        taskData->state = "Exception";
        return task::completed;
    }

    std::string index = std::to_string(taskData->index);
    taskData->messages.emplace_back(taskEntry);
    taskData->messages.emplace_back(messages::taskCompletedOK(index));
    taskData->state = "Completed";

    return task::completed;
}

/**
 * @brief Create an RDE operation task and start its timer.
 *
 * This function sets up a task to monitor RDE operation updates via D-Bus
 * signals. It uses the provided object path to build a match rule and
 * associates the task with the given resource data and payload.
 *
 * @param payload      The task payload containing response and context.
 * @param objPath      The D-Bus object path for the RDE operation.
 * @param resourceData Metadata and schema information for the RDE resource.
 */

inline void rdeCreateTask(task::Payload&& payload,
                          const sdbusplus::message::object_path& objPath,
                          const RDESchemaResource& resourceData)
{
    std::string matchString = rdeOpTaskMatch(objPath.str);

    nlohmann::json uriMapJson;
    for (const auto& it : resourceData)
    {
        try
        {
            int resourceId = std::stoi(it.rid);
            uriMapJson[std::to_string(resourceId)] = it.fullUri;
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_WARNING("RDE: Invalid rid:{}  error:{} ", it.rid,
                               e.what());
        }
    }

    // Embed uriMap into payload metadata
    payload.jsonBody["uriMap"] = std::move(uriMapJson);

    auto task = task::TaskData::createTask(
        std::bind_front(redfish::rdeProcessTaskUpdateSignal), matchString);

    task->startTimer(std::chrono::minutes(rdeTaskTimeoutMinutes));
    task->payload.emplace(std::move(payload));
}

/**
 * @class RDEServiceHandler
 * @brief Handles Redfish Device Enablement (RDE) service requests.
 *
 * This class is responsible for managing the lifecycle of an RDE request,
 * including extracting URI components, validating metadata, and initiating
 * schema-specific processing logic.
 *
 * It supports both root-level and vendor-specific Redfish endpoints for
 * devices such as processors and network adapters. The handler is initialized
 * with request context, schema, and device identifiers, and is designed to
 * operate asynchronously using the Redfish bmcweb framework.
 *
 * Key Responsibilities:
 * - Parse and store URI components (system ID, device ID, schema).
 * - Construct base and sub URIs for Redfish resource access.
 * - Extract and validate UUIDs from device metadata.
 * - Kick off the processing flow via `bootstrapProcess()`.
 *
 * The class is intended to be used via `std::shared_ptr` and supports
 * move semantics while disallowing copy operations.
 *
 * @note Use `bootstrapProcess()` after construction to begin request handling.
 */
class RDEServiceHandler : public std::enable_shared_from_this<RDEServiceHandler>
{
  public:
    RDEServiceHandler() = default;
    RDEServiceHandler(RDEServiceHandler&&) = default;
    RDEServiceHandler& operator=(RDEServiceHandler&&) = default;
    RDEServiceHandler(const RDEServiceHandler&) = delete;
    RDEServiceHandler& operator=(const RDEServiceHandler&) = delete;
    /**
     * @brief Constructs an RDEServiceHandler instance for handling RDE service
     * requests.
     *
     * This constructor initializes the handler with request context, device
     * identifiers, schema information, and request type (root or
     * vendor-specific). It sets up internal state required for processing
     * Redfish Device Enablement (RDE) requests.
     *
     * High-Level Approach:
     * - Captures the HTTP request and asynchronous response context.
     * - Stores identifiers for the system, device, and schema.
     * - Constructs the base URI for the Redfish resource using the schema and
     * device ID.
     * - Initializes internal members such as the UUID list, selected UUID, and
     * request type.
     * - Prepares the handler for metadata extraction and subsequent processing
     * via `bootstrapProcess()`.
     *
     * @param[in] app Reference to the main application object managing HTTP
     * routes.
     * @param[in] req Incoming HTTP request object.
     * @param[in] resp Shared pointer to the asynchronous response handler.
     * @param[in] systemIdIn Identifier for the system.
     * @param[in] deviceIdIn Identifier for the device.
     * @param[in] schemaIn Schema name associated with the device.
     * @param[in] baseUriIn  Base URI.
     * @param[in] isRootRequest Flag indicating whether the request targets the
     * root endpoint.
     */
    RDEServiceHandler(App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& resp,
                      const std::string& systemIdIn,
                      const std::string& deviceIdIn,
                      const std::string& schemaIn, const std::string& baseUriIn,
                      bool isRootRequest) :
        systemId(systemIdIn), deviceId(deviceIdIn), schema(schemaIn),
        baseUri(baseUriIn), subUri{}, deviceUUID{}, deviceUUIDList{},
        deviceEID{0}, responseSent{false}, verb(req.method()),
        isRoot(isRootRequest), request(req), asyncResp(resp)

    {
        if (!redfish::setUpRedfishRoute(app, request, asyncResp))
        {
            // TODO Add Error Hnadler
            return;
        }

        if (!isRoot)
        {
            std::string fullUri = std::string(req.target());
            std::string remaining = fullUri.substr(baseUri.length());
            subUri = trimLeadingSlash(remaining);
        }

        if (!loadUUIDs())
        {
            // TODO revisit the Error handling
            BMCWEB_LOG_ERROR("RDE: Failied to get UUID for the requested uri");
            throw std::runtime_error("Initialization failed");
        }
        BMCWEB_LOG_DEBUG(
            "RDE: systemId={}, schema={}, deviceId={}, baseUri={}, subUri={}, isRoot={}",
            systemId, schema, deviceId, baseUri, subUri, isRoot);
    }

    /* @brief Destructor for RDEServiceHandler.
     *
     * Finalizes the RDE service request by updating the asynchronous response
     * object if necessary. This ensures that any deferred response logic is
     * completed when the handler goes out of scope.
     *
     * Design Note:
     * - The destructor is used to guarantee that the response is updated
     * exactly once, even in cases where early exits or errors prevent normal
     * flow.
     * - This pattern is useful in asynchronous Redfish handlers where response
     *   finalization may depend on multiple conditional paths.
     */
    ~RDEServiceHandler()
    {
        prepareAndSendResourceResponse();
        BMCWEB_LOG_INFO("RDEServiceHandler destroyed");
    }

    /**
     * @brief Initiates the RDE service request processing flow.
     *
     * This method begins the asynchronous handling of an RDE request by first
     * retrieving available RDE device objects from D-Bus. It extracts relevant
     * metadata properties and matches the device UUID against the internally
     * populated `deviceUUIDList`, which was previously loaded from the metadata
     * file.
     *
     * Design Assumptions:
     * - For a given device key (`schema + "/" + deviceId`), only one matching
     *   device object is expected to be present on D-Bus.
     * - If this assumption changes (e.g., multiple matching objects), the logic
     *   must be revisited to handle ambiguity or selection criteria.
     *
     * High-Level Flow:
     * - Initiate an asynchronous D-Bus query to enumerate RDE device objects.
     * - For each object, extract required metadata properties (e.g., UUID).
     * - Match the UUID against `deviceUUIDList` to identify the correct device.
     * - Once a match is found, store relevant properties into class members.
     * - Trigger the next stage of request processing only after the initial
     *   asynchronous D-Bus call completes successfully.
     *
     * This method follows a staged asynchronous design pattern, where each
     * operation (e.g., D-Bus query, property extraction, UUID matching) is
     * performed in sequence, and the next step is initiated only after the
     * previous one completes. This ensures non-blocking behavior and proper
     * resource handling in the Redfish request lifecycle.
     */
    inline void bootstrapProcess()
    {
        BMCWEB_LOG_DEBUG("RDE: bootstrapProcess: Enter Device ID {}", deviceId);

        auto self =
            shared_from_this(); // Capture shared_ptr to keep object alive
        constexpr std::array<std::string_view, 1> interfaces = {
            rdeDeviceInterface};

        dbus::utility::getSubTreePaths(
            "/", 0, interfaces,
            [self](
                const boost::system::error_code& ec,
                const dbus::utility::MapperGetSubTreePathsResponse& objPaths) {
            if (ec)
            {
                BMCWEB_LOG_ERROR(
                    "RDE: bootstrapProcess: getSubTreePaths DBUS error: {}",
                    ec.message());
                messages::internalError(self->asyncResp->res);
                return;
            }

            for (const std::string& path : objPaths)
            {
                sdbusplus::message::object_path objPath(path);
                std::string pathName = objPath.filename();
                if (pathName.empty())
                {
                    BMCWEB_LOG_ERROR(
                        "RDE: bootstrapProcess: Failed to find '/' in {}",
                        path);
                    continue;
                }

                BMCWEB_LOG_DEBUG(
                    "RDE: bootstrapProcess: Found device with ObjectPath {}",
                    path);

                sdbusplus::asio::getAllProperties(
                    *crow::connections::systemBus, std::string(pldmService),
                    path, std::string(rdeDeviceInterface),
                    [self](const boost::system::error_code& errCode,
                           const RDEPropertiesMap& properties) {
                    if (errCode)
                    {
                        BMCWEB_LOG_ERROR(
                            "RDE: bootstrapProcess: DBUS response error: {}",
                            errCode.message());
                        return;
                    }

                    std::string uuid;
                    SchemaResourcesType schemaResources;

                    const bool success = sdbusplus::unpackPropertiesNoThrow(
                        dbus_utils::UnpackErrorPrinter(), properties,
                        "DeviceUUID", uuid, "EID", self->deviceEID,
                        "SchemaResources", schemaResources);

                    if (!success)
                    {
                        BMCWEB_LOG_ERROR(
                            "RDE: bootstrapProcess: Failed to unpack properties");
                        return;
                    }

                    if (std::find(self->deviceUUIDList.begin(),
                                  self->deviceUUIDList.end(),
                                  uuid) == self->deviceUUIDList.end())
                    {
                        BMCWEB_LOG_DEBUG(
                            "RDE: bootstrapProcess: UUID mismatch: expected");
                        return;
                    }
                    // UUID matched — update the active device UUID
                    self->deviceUUID = uuid;
                    BMCWEB_LOG_INFO(
                        "RDE: bootstrapProcess: UUID {} matched and updated",
                        uuid);

                    for (const auto& [rid, valueMap] : schemaResources)
                    {
                        RDESchemaEntry entry;

                        entry.rid = rid;
                        if (const auto* val = std::get_if<std::string>(
                                &valueMap.at("schemaName")))
                            entry.schemaName = *val;
                        if (const auto* val = std::get_if<std::string>(
                                &valueMap.at("schemaVersion")))
                            entry.schemaVersion = *val;
                        if (const auto* val = std::get_if<std::string>(
                                &valueMap.at("subUri")))
                            entry.subUri = *val;
                        if (const auto* val = std::get_if<int64_t>(
                                &valueMap.at("schemaClass")))
                            entry.schemaClass = *val;
                        if (!entry.subUri.empty())
                        {
                            entry.fullUri = self->baseUri + "/" + entry.subUri;
                        }

                        // Add to resourceData
                        self->resourceData.push_back(std::move(entry));
                    }
                    BMCWEB_LOG_INFO(
                        "RDE: bootstrapProcess: Successfully updated resourceData for UUID {}",
                        uuid);
                    self->process(self);
                });
            }
        });
    }

  private:
    /**
     * @brief Removes a leading slash from the input string, if present.
     *
     * This utility function checks whether the input string begins with a '/'
     * character and returns a new string with the leading slash removed. If no
     * leading slash is present, the original string is returned unchanged.
     *
     * @param[in] s Input string to be trimmed.
     * @return A copy of the input string without a leading slash.
     */
    inline std::string trimLeadingSlash(const std::string& s)
    {
        return (!s.empty() && s[0] == '/') ? s.substr(1) : s;
    }

    /**
     * @brief Extracts the UUID for a device based on schema and deviceId.
     *
     * This method reads the device metadata from a JSON file located at
     * `/etc/pldm/rde_device_metadata.json`. It uses the class member variables
     * `schema` and `deviceId` to locate the corresponding section in the JSON.
     *
     * The expected structure of the JSON is:
     * {
     *   "SchemaName": {
     *     "DeviceId": {
     *       "UUIDs": ["uuid1", "uuid2", ...]
     *     }
     *   }
     * }
     *
     * Algorithm:
     * - Check if the metadata file exists and is readable.
     * - Parse the JSON and validate its structure.
     * - Navigate to jsonData[schema][deviceId]["UUIDs"].
     * - If valid, extract all UUID strings from the array.
     * - Store the extracted UUIDs in the class member `deviceUUIDList`.
     *
     * @return true if a valid UUID was found and assigned, false otherwise.
     */

    inline bool loadUUIDs()
    {
        constexpr const char* rdeDeviceMetadataFile =
            "/etc/pldm/rde_device_metadata.json";

        if (!std::filesystem::exists(rdeDeviceMetadataFile))
        {
            BMCWEB_LOG_ERROR("RDE: Device metadata file {} not found: ",
                             rdeDeviceMetadataFile);
            return false;
        }

        std::ifstream file(rdeDeviceMetadataFile);
        if (!file.is_open())
        {
            BMCWEB_LOG_ERROR("RDE: Failed to open device metadata file:{} ",
                             rdeDeviceMetadataFile);
            return false;
        }

        try
        {
            if (file.peek() == std::ifstream::traits_type::eof())
            {
                BMCWEB_LOG_ERROR("RDE: Device metadata file{} is empty: ",
                                 rdeDeviceMetadataFile);
                return false;
            }

            nlohmann::json jsonData;
            file >> jsonData;

            if (!jsonData.is_object())
            {
                BMCWEB_LOG_ERROR(
                    "RDE: Device metadata file does not contain a valid JSON object.");
                return false;
            }

            const std::string expectedDeviceKey = schema + "/" + deviceId;
            for (const auto& [jsonSchema, deviceEntries] : jsonData.items())
            {
                if (!deviceEntries.is_object())
                {
                    BMCWEB_LOG_ERROR("RDE: Invalid schema section {} ",
                                     jsonSchema);
                    continue;
                }

                for (const auto& [jsonDeviceId, deviceInfo] :
                     deviceEntries.items())
                {
                    if (!deviceInfo.contains("UUIDs") ||
                        !deviceInfo["UUIDs"].is_array())
                    {
                        BMCWEB_LOG_ERROR(
                            "RDE: Missing or invalid 'UUIDs' for {} {}",
                            jsonSchema, jsonDeviceId);
                        continue;
                    }

                    const std::string deviceKeyFromJson = jsonSchema + "/" +
                                                          jsonDeviceId;

                    if (deviceKeyFromJson == expectedDeviceKey)
                    {
                        for (const auto& uuid : deviceInfo["UUIDs"])
                        {
                            if (!uuid.is_string())
                            {
                                BMCWEB_LOG_ERROR(
                                    "RDE: Invalid UUID format in {}",
                                    deviceKeyFromJson);
                                continue;
                            }
                            deviceUUIDList.push_back(uuid.get<std::string>());
                        }

                        if (!deviceUUIDList.empty())
                        {
                            BMCWEB_LOG_INFO(
                                "RDE: Device UUIDs initialized: count={}",
                                deviceUUIDList.size());
                            return true;
                        }
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            BMCWEB_LOG_ERROR(
                "RDE: Unexpected error while reading device metadata file:{} ",
                e.what());
            return false;
        }

        return false;
    }

    /**
     * @brief Prepares and sends a schema-specific Redfish resource response.
     *
     * This method constructs the standard Redfish collection response for a
     * given schema (e.g., "Processors", "NetworkAdapter") by setting
     * fields such as
     * @odata.type, @odata.id, Name, Members, and Members@odata.count. It uses
     * the internal `schema` and `baseUri` to determine the appropriate response
     * format.
     *
     * On success:
     * - The response includes a correctly typed collection with an empty
     * Members array.
     * - The Members@odata.count is set based on the size of `deviceUUIDList`.
     *
     * On fallback:
     * - If the schema is unrecognized, a generic collection type is used.
     * - Logs are generated to aid debugging and future schema support.
     *
     * This function finalizes the response using `asyncResp` and ensures it is
     * sent only once, following the asynchronous Redfish request handling
     * pattern.
     */

    inline void prepareAndSendResourceResponse()
    {
        if (responseSent)
            return;

        std::string odataType;
        std::string resourceName;

        if (schema == procSchemaName)
        {
            odataType = "#AmdSocConfiguration.AmdSocConfiguration";
            resourceName = "AMD SoC Configuration";
        }
        else if (schema == networkSchemaName)
        {
            odataType = "#NetworkAdapter.v1_0_0.NetworkAdapter";
            resourceName = "Network Adapter";
        }
        else
        {
            BMCWEB_LOG_WARNING(
                "RDE: Unknown schema '{}', using generic fallback", schema);
            odataType = "#Resource.Resource";
            resourceName = schema + " Resource";
        }

        auto& json = asyncResp->res.jsonValue;
        json["@odata.type"] = odataType;
        json["@odata.id"] = baseUri;
        json["Name"] = resourceName;

        asyncResp->res.result(boost::beast::http::status::ok);
        responseSent = true;
    }

    /**
     * @brief Dispatches HTTP request handling based on verb and context.
     *
     * This function acts as the central entry point for handling Redfish
     * requests in the RDE service handler. It interprets the HTTP verb and
     * the `isRoot` flag to determine the appropriate response strategy.
     *
     * - For GET requests:
     *   - If `isRoot` is true, it builds a resource metadata response via
     * buildResourceResponse().
     *   - If `isRoot` is false, no action is taken; the destructor sends a
     * fallback collection response.
     * - For PATCH requests:
     *   - Responds with a placeholder indicating unsupported Time-of-Day (TOD)
     * logic.
     * - For unsupported verbs:
     *   - Responds with `MethodNotAllowed`.
     *
     * @param[in] self Shared pointer to the current RDEServiceHandler instance.
     */
    inline void process(const std::shared_ptr<RDEServiceHandler>& self)
    {
        if (verb == boost::beast::http::verb::get)
        {
            BMCWEB_LOG_DEBUG("RDE: Handling GET for device {}", deviceUUID);

            if (isRoot)
            {
                // Root GET: build resource metadata
                self->buildResourceResponse(self);
                return;
            }
            operationGet(self);
        }
        else if (verb == boost::beast::http::verb::patch)
        {
            BMCWEB_LOG_DEBUG("RDE: Handling PATCH for device {}", deviceId);
            operationPatch(self);
        }
        else
        {
            BMCWEB_LOG_ERROR("RDE: Unsupported HTTP verb");
            messages::operationNotAllowed(asyncResp->res);
        }
    }

    /**
     * @brief Constructs and sends a resource metadata response for root GET
     * requests.
     *
     * This function builds a Redfish-style JSON response containing metadata
     * for each sub-resource registered in `resourceData`. It is specifically
     * invoked when handling a GET request where `isRoot == true`.
     *
     * Each entry in `resourceData` is converted into a JSON object with:
     * - `@odata.id`: URI to the sub-resource
     * - `SchemaName`: Redfish schema name
     * - `SchemaVersion`: Schema version
     * - `SchemaClass`: Logical class/category of the resource
     *
     * The response is sent with HTTP 200 OK and includes:
     * - `@odata.id`: Base URI
     * - `Members`: Array of sub-resource metadata
     * - `Members@odata.count`: Count of members
     *
     * @param[in] self Shared pointer to the current RDEServiceHandler instance.
     */
    inline void
        buildResourceResponse(const std::shared_ptr<RDEServiceHandler>& self)
    {
        if (self->responseSent)
            return;

        nlohmann::json& members = self->asyncResp->res.jsonValue["Members"];
        if (!members.is_array())
        {
            members = nlohmann::json::array();
        }

        for (const auto& entry : self->resourceData)
        {
            nlohmann::json memberEntry;
            memberEntry["@odata.id"] = entry.fullUri;
            memberEntry["SchemaName"] = entry.schemaName;
            memberEntry["SchemaVersion"] = entry.schemaVersion;
            memberEntry["SchemaClass"] = entry.schemaClass;
            memberEntry["ResourceId"] = entry.rid; // Optional
            members.push_back(std::move(memberEntry));
        }

        self->asyncResp->res.jsonValue["Members@odata.count"] =
            static_cast<int>(members.size());
        self->asyncResp->res.result(boost::beast::http::status::ok);
    }

    /**
     * @brief Returns the next RDE operation ID.
     *
     * Reads the last operation ID from the file (if it exists),
     * increments it, writes the new value back to the file,
     * and returns the incremented ID.
     *
     * @return The next sequential operation ID.
     */
    uint32_t nextOperationId()
    {
        uint32_t operationId = 0;
        if (std::filesystem::exists(operationIdFile))
        {
            std::ifstream inFile(operationIdFile);
            inFile >> operationId;
        }
        std::ofstream outFile(operationIdFile);
        outFile << ++operationId;

        return operationId;
    }

    /**
     * @brief Initiates a GET-based RDE Redfish operation via DBus.
     *
     * This function triggers the RDE Manager to start a Redfish GET operation
     * using metadata stored in the RDEServiceHandler instance. It sets up
     * asynchronous DBus handling and a fallback timer for completion.
     *
     * @param[in] self Shared pointer to the RDEServiceHandler instance.
     *
     * @details
     * Algorithm Flow:
     * 1. Generate a boot-unique operation ID.
     * 2. Call DBus method `StartRedfishOperation`.
     * 3. Set up fallback timer for timeout.
     * 4. Register signal match for `TaskUpdated`.
     * 5. Parse payload and update response.
     * 6. Response finalization is handled externally.
     */
    inline void operationGet(const std::shared_ptr<RDEServiceHandler>& self)
    {
        BMCWEB_LOG_INFO("RDE GET operation for system [{}]", self->deviceUUID);
        constexpr const char* rdeEmptyPayload = "{}";

        // Begin async method call
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   const sdbusplus::message_t&, const ObjectPath& objPath) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("RDE: operationGet: {}", ec.message());
                messages::internalError(self->asyncResp->res);
                return;
            }

            BMCWEB_LOG_INFO("RDE: task created at {}",
                            static_cast<const std::string&>(objPath));

            self->operationTaskTimeout =
                std::make_shared<boost::asio::steady_timer>(
                    crow::connections::systemBus->get_io_context());
            self->operationTaskTimeout->expires_after(std::chrono::seconds(10));
            self->operationTaskTimeout->async_wait(
                [self](const boost::system::error_code& timerEc) {
                if (timerEc != boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_ERROR("RDE: Timeout — No signal received.");
                    messages::internalError(self->asyncResp->res);
                    return;
                }
                else
                {
                    BMCWEB_LOG_INFO("RDE: Timer cancelled — signal processed.");
                    return;
                }
            });

            const std::string matchRule = rdeOpTaskMatch(objPath.str);
            self->operationTaskSignalMatch =
                std::make_shared<sdbusplus::bus::match::match>(
                    *crow::connections::systemBus, matchRule,
                    [self](sdbusplus::message_t& msg) {
                BMCWEB_LOG_DEBUG(
                    "RDE: Signal received for OperationTask update");
                std::unordered_map<std::string, DbusVariantType> changed;

                try
                {
                    msg.read(changed);
                }
                catch (const std::exception& e)
                {
                    BMCWEB_LOG_ERROR("RDE: Failed to read DBus signal: {}",
                                     e.what());
                    return;
                }

                auto codeIt = changed.find("CompletionCode");
                if (codeIt != changed.end())
                {
                    uint16_t code = std::get<uint16_t>(codeIt->second);
                    if (code == completionCodeOperationCompleted)
                    {
                        BMCWEB_LOG_INFO("RDE: Task completed successfully.");
                    }
                    else if (code == completionCodeOperationFailed)
                    {
                        BMCWEB_LOG_ERROR("RDE: Task failed.");
                        messages::internalError(self->asyncResp->res);
                    }
                    else
                    {
                        BMCWEB_LOG_INFO(
                            "RDE: Interim signal received with code {}", code);
                        return;
                    }
                }

                auto payloadIt = changed.find("payload");
                if (payloadIt != changed.end())
                {
                    const std::string& payloadStr =
                        std::get<std::string>(payloadIt->second);
                    try
                    {
                        nlohmann::json uriMapJson;
                        for (const auto& it : self->resourceData)
                        {
                            try
                            {
                                int resourceId = std::stoi(it.rid);
                                uriMapJson[std::to_string(resourceId)] =
                                    it.fullUri;
                            }
                            catch (const std::exception& e)
                            {
                                BMCWEB_LOG_WARNING(
                                    "RDE: Invalid rid:{}  error:{} ", it.rid,
                                    e.what());
                            }
                        }
                        self->asyncResp->res.jsonValue["Payload"] =
                            redfish::handleDeferredBindings(payloadStr,
                                                            uriMapJson);
                        BMCWEB_LOG_INFO(
                            "RDE: Parsed payload and updated response");
                    }
                    catch (const std::exception& e)
                    {
                        BMCWEB_LOG_ERROR("RDE: Payload parse error: {}",
                                         e.what());
                        messages::internalError(self->asyncResp->res);
                    }
                }

                self->operationTaskTimeout->cancel();
                self->operationTaskTimeout.reset();
                self->operationTaskSignalMatch.reset();
                return;
            });
        },
            pldmService, rdeManagerPath, rdeManagerInterface,
            rdeOperationMethod, self->nextOperationId(), rdeOperationTypeRead,
            self->subUri, self->deviceUUID, self->deviceEID, rdeEmptyPayload,
            rdePayloadFormatInline, rdeEncodingFormatJSON, self->schema);
    }

    /**
     * @brief Initiates an RDE PATCH operation for a system.
     *
     * This function parses the incoming request body as JSON, extracts the
     * payload, and initiates an asynchronous D-Bus method call to start an RDE
     * Patch operation. Upon success, it creates a task and embeds the resource
     * metadata.
     *
     * @param self Shared pointer to the RDEServiceHandler instance containing
     *             device context and schema information.
     */
    inline void operationPatch(const std::shared_ptr<RDEServiceHandler>& self)
    {
        BMCWEB_LOG_INFO("RDE GET operation for system [{}]", self->deviceUUID);

        nlohmann::json jsonPayload;
        try
        {
            jsonPayload = nlohmann::json::parse(self->request.body());
        }
        catch (const nlohmann::json::parse_error& e)
        {
            BMCWEB_LOG_ERROR("RDE: JSON parse error: {}", e.what());
            messages::malformedJSON(self->asyncResp->res);
            return;
        }

        std::string rdePayload = jsonPayload.dump();
        // Begin async method call
        crow::connections::systemBus->async_method_call(
            [self](const boost::system::error_code ec,
                   const sdbusplus::message_t&, const ObjectPath& objPath) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("RDE: operationGet failed: {}", ec.message());
                messages::internalError(self->asyncResp->res);
                return;
            }

            BMCWEB_LOG_INFO("RDE: task created at {}",
                            static_cast<const std::string&>(objPath));

            task::Payload payload(self->request);
            rdeCreateTask(std::move(payload), objPath, self->resourceData);
        },
            pldmService, rdeManagerPath, rdeManagerInterface,
            rdeOperationMethod, self->nextOperationId(), rdeOperationTypePatch,
            self->subUri, self->deviceUUID, self->deviceEID, rdePayload,
            rdePayloadFormatInline, rdeEncodingFormatJSON, self->schema);
    }

    /**
     * @brief Internal state and context used for handling RDE service requests.
     *
     * These member variables store identifiers, URI components, metadata, and
     * control flags required to process Redfish Device Enablement (RDE)
     * requests. They are initialized during construction and updated throughout
     * the request lifecycle, supporting asynchronous D-Bus interactions and
     * UUID-based device matching.
     */
    std::string systemId;
    std::string deviceId;
    std::string schema;
    std::string baseUri;
    std::string subUri;
    std::string deviceUUID;
    std::vector<std::string> deviceUUIDList;
    uint8_t deviceEID;
    RDESchemaResource resourceData;
    bool responseSent;
    // Holds the signal match for OperationTask completion
    std::shared_ptr<sdbusplus::bus::match::match> operationTaskSignalMatch;
    // Timer to handle timeout if no signal is received
    std::shared_ptr<boost::asio::steady_timer> operationTaskTimeout;

    boost::beast::http::verb verb;
    bool isRoot;
    const crow::Request& request;
    std::shared_ptr<bmcweb::AsyncResp> asyncResp;
};

/**
 * @brief Handles the root RDE service request for a specific device.
 *
 * Initializes an RDEServiceHandler instance using the provided request context
 * and device identifiers, and triggers the bootstrap process. This is typically
 * used to handle requests to the RDE service root endpoint.
 *
 * @param[in] app Reference to the main application object managing HTTP routes.
 * @param[in] req Incoming HTTP request object.
 * @param[in] asyncResp Shared pointer to the asynchronous response handler.
 * @param[in] systemId Identifier for the system.
 * @param[in] deviceId Identifier for the device.
 * @param[in] schema Schema name associated with the device.
 * @param[in] baseUri Base URI.
 */
inline void
    handleRDEServiceRoot(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemId,
                         const std::string& deviceId, const std::string& schema,
                         const std::string& baseUri)
{
    BMCWEB_LOG_INFO("RDE: handleRDEServiceRoot Enter");

    auto handler = std::shared_ptr<RDEServiceHandler>(new RDEServiceHandler(
        app, req, asyncResp, systemId, deviceId, schema, baseUri, true));

    handler->bootstrapProcess();
}

/**
 * @brief Handles the RDE service request for a vendor-specific path.
 *
 * Initializes an RDEServiceHandler instance using the provided request context
 * and device identifiers, and triggers the bootstrap process. This is typically
 * used to handle requests to vendor-specific RDE service paths.
 *
 * @param[in] app Reference to the main application object managing HTTP routes.
 * @param[in] req Incoming HTTP request object.
 * @param[in] asyncResp Shared pointer to the asynchronous response handler.
 * @param[in] systemId Identifier for the system.
 * @param[in] deviceId Identifier for the device.
 * @param[in] vendorPath Vendor-specific path segment (currently unused).
 * @param[in] schema Schema name associated with the device.
 * @param[in] baseUri Base URI.
 */
inline void
    handleRDEServicePath(App& app, const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& systemId,
                         const std::string& deviceId,
                         const std::string& vendorPath,
                         const std::string& schema, const std::string& baseUri)
{
    BMCWEB_LOG_INFO("RDE: handleRDEServicePath Enter");
    (void)vendorPath; // Silence unused parameter warning if not used

    auto handler = std::shared_ptr<RDEServiceHandler>(new RDEServiceHandler(
        app, req, asyncResp, systemId, deviceId, schema, baseUri, false));

    handler->bootstrapProcess();
}

/**
 * @brief Registers Redfish route handlers for RDE service endpoints.
 *
 * This function sets up Redfish-compliant HTTP routes for handling RDE
 * (Redfish Device Enablement) service requests related to AMD-specific
 * processor and network adapter configurations. It binds route handlers
 * for both root-level and vendor-specific paths under the Redfish schema.
 *
 * High-Level Approach:
 * - Define schema names for supported device types (e.g., "Processors",
 * "NetworkAdapters").
 * - Register route handlers for both root and vendor-specific paths using
 * `BMCWEB_ROUTE`.
 * - For each route, bind the appropriate handler (`handleRDEServiceRoot` or
 * `handleRDEServicePath`) with the correct schema context.
 * - Each handler constructs an `RDEServiceHandler` instance and invokes
 * `bootstrapProcess()` to initiate metadata collection and processing flow.
 *
 * @param[in,out] app Reference to the Crow application instance used to
 * register routes.
 */
inline void requestRoutesRDEService(crow::App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/Oem/AMD/SocConfiguration/<path>")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::head, boost::beast::http::verb::get,
                 boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemId, const std::string& deviceId,
                   const std::string& vendorPath) {
        std::string baseUri = "/redfish/v1/Systems/" + systemId +
                              "/Processors/" + deviceId;

        handleRDEServicePath(app, req, asyncResp, systemId, deviceId,
                             vendorPath, redfish::procSchemaName, baseUri);
    });

    BMCWEB_ROUTE(
        app,
        "/redfish/v1/Systems/<str>/Processors/<str>/Oem/AMD/SocConfiguration")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemId, const std::string& deviceId) {
        std::string baseUri = "/redfish/v1/Systems/" + systemId +
                              "/Processors/" + deviceId;
        handleRDEServiceRoot(app, req, asyncResp, systemId, deviceId,
                             redfish::procSchemaName, baseUri);
    });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/NetworkAdapters/<str>/<path>")
        .privileges(redfish::privileges::patchProcessor)
        .methods(boost::beast::http::verb::head, boost::beast::http::verb::get,
                 boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemId, const std::string& deviceId,
                   const std::string& vendorPath) {
        std::string baseUri = "/redfish/v1/Systems/" + systemId +
                              "/NetworkAdapters/" + deviceId;

        handleRDEServicePath(app, req, asyncResp, systemId, deviceId,
                             vendorPath, redfish::networkSchemaName, baseUri);
    });

    BMCWEB_ROUTE(app, "/redfish/v1/Systems/<str>/NetworkAdapters/<str>")
        .privileges(redfish::privileges::getProcessor)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const std::string& systemId, const std::string& deviceId) {
        std::string baseUri = "/redfish/v1/Systems/" + systemId +
                              "/NetworkAdapters/" + deviceId;
        handleRDEServiceRoot(app, req, asyncResp, systemId, deviceId,
                             redfish::networkSchemaName, baseUri);
    });
}

} // namespace redfish
