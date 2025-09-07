#pragma once

#include "app.hpp"
#include "http_utility.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"
#include "registries/openbmc_message_registry.hpp"
#include "registries/privilege_registry.hpp"

#include <bmcweb_config.h>

#include <boost/algorithm/string.hpp>
#include <boost/beast/http.hpp>
#include <boost/url/format.hpp>
#include <nlohmann/json.hpp>
#include <utils/json_utils.hpp>

#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace redfish
{
constexpr const char* procSchemaName = "Processors";
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
        BMCWEB_LOG_INFO("bootstrapProcess :Enter");
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
}

} // namespace redfish
