#pragma once

#include "aggregation_utils.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http_client.hpp"
#include "http_connection.hpp"
#include "parsing.hpp"

#include <array>
#include <ranges>
#include <string_view>

namespace redfish
{

constexpr unsigned int aggregatorReadBodyLimit = 50 * 1024 * 1024; // 50MB

enum class Result
{
    LocalHandle,
    NoLocalHandle
};

enum class SearchType
{
    Collection,
    CollOrCon,
    ContainsSubordinate,
    Resource
};

// clang-format off
// These are all of the properties as of version 2022.2 of the Redfish Resource
// and Schema Guide whose Type is "string (URI)" and the name does not end in a
// case-insensitive form of "uri".  That version of the schema is associated
// with version 1.16.0 of the Redfish Specification.  Going forward, new URI
// properties should end in URI so this list should not need to be maintained as
// the spec is updated.  NOTE: These have been pre-sorted in order to be
// compatible with binary search
constexpr std::array nonUriProperties{
    "@Redfish.ActionInfo",
    // "@odata.context", // We can't fix /redfish/v1/$metadata URIs
    "@odata.id",
    // "Destination", // Only used by EventService and won't be a Redfish URI
    // "HostName", // Isn't actually a Redfish URI
    "Image",
    "MetricProperty",
    // "OriginOfCondition", // Is URI when in request, but is object in response
    "TaskMonitor",
    "target", // normal string, but target URI for POST to invoke an action
};
// clang-format on

// Search the top collection array to determine if the passed URI is of a
// desired type
inline bool searchCollectionsArray(std::string_view uri,
                                   const SearchType searchType)
{
    constexpr std::string_view serviceRootUri = "/redfish/v1";

    // The passed URI must begin with "/redfish/v1", but we have to strip it
    // from the URI since topCollections does not include it in its URIs
    if (!uri.starts_with(serviceRootUri))
    {
        return false;
    }

    // Catch empty final segments such as "/redfish/v1/Chassis//"
    if (uri.ends_with("//"))
    {
        return false;
    }

    std::size_t parseCount = uri.size() - serviceRootUri.size();
    // Don't include the trailing "/" if it exists such as in "/redfish/v1/"
    if (uri.ends_with("/"))
    {
        parseCount--;
    }

    boost::system::result<boost::urls::url_view> parsedUrl =
        boost::urls::parse_relative_ref(
            uri.substr(serviceRootUri.size(), parseCount));
    if (!parsedUrl)
    {
        BMCWEB_LOG_ERROR("Failed to get target URI from {}",
                         uri.substr(serviceRootUri.size()));
        return false;
    }

    if (!parsedUrl->segments().is_absolute() && !parsedUrl->segments().empty())
    {
        return false;
    }

    // If no segments() then the passed URI was either "/redfish/v1" or
    // "/redfish/v1/".
    if (parsedUrl->segments().empty())
    {
        return (searchType == SearchType::ContainsSubordinate) ||
               (searchType == SearchType::CollOrCon);
    }
    std::string_view url = parsedUrl->buffer();
    const auto* it = std::ranges::lower_bound(topCollections, url);
    if (it == topCollections.end())
    {
        // parsedUrl is alphabetically after the last entry in the array so it
        // can't be a top collection or up tree from a top collection
        return false;
    }

    boost::urls::url collectionUrl(*it);
    boost::urls::segments_view collectionSegments = collectionUrl.segments();
    boost::urls::segments_view::iterator itCollection =
        collectionSegments.begin();
    const boost::urls::segments_view::const_iterator endCollection =
        collectionSegments.end();

    // Each segment in the passed URI should match the found collection
    for (const auto& segment : parsedUrl->segments())
    {
        if (itCollection == endCollection)
        {
            // Leftover segments means the target is for an aggregation
            // supported resource
            return searchType == SearchType::Resource;
        }

        if (segment != (*itCollection))
        {
            return false;
        }
        itCollection++;
    }

    // No remaining segments means the passed URI was a top level collection
    if (searchType == SearchType::Collection)
    {
        return itCollection == endCollection;
    }
    if (searchType == SearchType::ContainsSubordinate)
    {
        return itCollection != endCollection;
    }

    // Return this check instead of "true" in case other SearchTypes get added
    return searchType == SearchType::CollOrCon;
}

// Determines if the passed property contains a URI.  Those property names
// either end with a case-insensitive version of "uri" or are specifically
// defined in the above array.
inline bool isPropertyUri(std::string_view propertyName)
{
    if (propertyName.ends_with("uri") || propertyName.ends_with("Uri") ||
        propertyName.ends_with("URI"))
    {
        return true;
    }
    return std::binary_search(nonUriProperties.begin(), nonUriProperties.end(),
                              propertyName);
}

static inline void addPrefixToStringItem(std::string& strValue,
                                         std::string_view prefix)
{
    // Make sure the value is a properly formatted URI
    auto parsed = boost::urls::parse_relative_ref(strValue);
    if (!parsed)
    {
        // Note that DMTF URIs such as
        // https://redfish.dmtf.org/registries/Base.1.15.0.json will fail this
        // check and that's okay
        BMCWEB_LOG_DEBUG("Couldn't parse URI from resource {}", strValue);
        return;
    }

    boost::urls::url_view thisUrl = *parsed;

    // We don't need to aggregate JsonSchemas due to potential issues such as
    // version mismatches between aggregator and satellite BMCs.  For now
    // assume that the aggregator has all the schemas and versions that the
    // aggregated server has.
    if (crow::utility::readUrlSegments(thisUrl, "redfish", "v1", "JsonSchemas",
                                       crow::utility::OrMorePaths()))
    {
        BMCWEB_LOG_DEBUG("Skipping JsonSchemas URI prefix fixing");
        return;
    }

    // The first two segments should be "/redfish/v1".  We need to check that
    // before we can search topCollections
    if (!crow::utility::readUrlSegments(thisUrl, "redfish", "v1",
                                        crow::utility::OrMorePaths()))
    {
        return;
    }

    // Check array adding a segment each time until collection is identified
    // Add prefix to segment after the collection
    const boost::urls::segments_view urlSegments = thisUrl.segments();
    bool addedPrefix = false;
    boost::urls::url url("/");
    boost::urls::segments_view::const_iterator it = urlSegments.begin();
    const boost::urls::segments_view::const_iterator end = urlSegments.end();

    // Skip past the leading "/redfish/v1"
    it++;
    it++;
    for (; it != end; it++)
    {
        // Trailing "/" will result in an empty segment.  In that case we need
        // to return so we don't apply a prefix to top level collections such
        // as "/redfish/v1/Chassis/"
        if ((*it).empty())
        {
            return;
        }

        if (std::binary_search(topCollections.begin(), topCollections.end(),
                               url.buffer()))
        {
            std::string collectionItem(prefix);
            collectionItem += "_" + (*it);
            url.segments().push_back(collectionItem);
            it++;
            addedPrefix = true;
            break;
        }

        url.segments().push_back(*it);
    }

    // Finish constructing the URL here (if needed) to avoid additional checks
    for (; it != end; it++)
    {
        url.segments().push_back(*it);
    }

    if (addedPrefix)
    {
        url.segments().insert(url.segments().begin(), {"redfish", "v1"});
        strValue = url.buffer();
    }
}

static inline void addPrefixToItem(nlohmann::json& item,
                                   std::string_view prefix)
{
    std::string* strValue = item.get_ptr<std::string*>();
    if (strValue == nullptr)
    {
        // Values for properties like "InvalidURI" and "ResourceMissingAtURI"
        // from within the Base Registry are objects instead of strings and will
        // fall into this check
        BMCWEB_LOG_DEBUG("Field was not a string");
        return;
    }
    addPrefixToStringItem(*strValue, prefix);
    item = *strValue;
}

static inline void addAggregatedHeaders(crow::Response& asyncResp,
                                        const crow::Response& resp,
                                        std::string_view prefix)
{
    if (!resp.getHeaderValue("Content-Type").empty())
    {
        asyncResp.addHeader(boost::beast::http::field::content_type,
                            resp.getHeaderValue("Content-Type"));
    }
    if (!resp.getHeaderValue("Allow").empty())
    {
        asyncResp.addHeader(boost::beast::http::field::allow,
                            resp.getHeaderValue("Allow"));
    }
    std::string_view header = resp.getHeaderValue("Location");
    if (!header.empty())
    {
        std::string location(header);
        addPrefixToStringItem(location, prefix);
        asyncResp.addHeader(boost::beast::http::field::location, location);
    }
    if (!resp.getHeaderValue("Retry-After").empty())
    {
        asyncResp.addHeader(boost::beast::http::field::retry_after,
                            resp.getHeaderValue("Retry-After"));
    }
    // TODO: we need special handling for Link Header Value
}

// Fix HTTP headers which appear in responses from Task resources among others
static inline void addPrefixToHeadersInResp(nlohmann::json& json,
                                            std::string_view prefix)
{
    // The passed in "HttpHeaders" should be an array of headers
    nlohmann::json::array_t* array = json.get_ptr<nlohmann::json::array_t*>();
    if (array == nullptr)
    {
        BMCWEB_LOG_ERROR("Field wasn't an array_t????");
        return;
    }

    for (nlohmann::json& item : *array)
    {
        // Each header is a single string with the form "<Field>: <Value>"
        std::string* strHeader = item.get_ptr<std::string*>();
        if (strHeader == nullptr)
        {
            BMCWEB_LOG_CRITICAL("Field wasn't a string????");
            continue;
        }

        constexpr std::string_view location = "Location: ";
        if (strHeader->starts_with(location))
        {
            std::string header = strHeader->substr(location.size());
            addPrefixToStringItem(header, prefix);
            *strHeader = std::string(location) + header;
        }
    }
}

// Search the json for all URIs and add the supplied prefix if the URI is for
// an aggregated resource.
static inline void addPrefixes(nlohmann::json& json, std::string_view prefix)
{
    nlohmann::json::object_t* object =
        json.get_ptr<nlohmann::json::object_t*>();
    if (object != nullptr)
    {
        for (std::pair<const std::string, nlohmann::json>& item : *object)
        {
            if (isPropertyUri(item.first))
            {
                addPrefixToItem(item.second, prefix);
                continue;
            }

            // "HttpHeaders" contains HTTP headers.  Among those we need to
            // attempt to fix the "Location" header
            if (item.first == "HttpHeaders")
            {
                addPrefixToHeadersInResp(item.second, prefix);
                continue;
            }

            // Recursively parse the rest of the json
            addPrefixes(item.second, prefix);
        }
        return;
    }
    nlohmann::json::array_t* array = json.get_ptr<nlohmann::json::array_t*>();
    if (array != nullptr)
    {
        for (nlohmann::json& item : *array)
        {
            addPrefixes(item, prefix);
        }
    }
}

inline boost::system::error_code aggregationRetryHandler(unsigned int respCode)
{
    // Allow all response codes because we want to surface any satellite
    // issue to the client
    BMCWEB_LOG_DEBUG("Received {} response from satellite", respCode);
    return boost::system::errc::make_error_code(boost::system::errc::success);
}

inline crow::ConnectionPolicy getAggregationPolicy()
{
    return {.maxRetryAttempts = 1,
            .requestByteLimit = aggregatorReadBodyLimit,
            .maxConnections = 20,
            .retryPolicyAction = "TerminateAfterRetries",
            .retryIntervalSecs = std::chrono::seconds(0),
            .invalidResp = aggregationRetryHandler};
}

class RedfishAggregator
{
  private:
    crow::HttpClient client;

    // Dummy callback used by the Constructor so that it can report the number
    // of satellite configs when the class is first created
    static void constructorCallback(
        const boost::system::error_code& ec,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        if (ec)
        {
            BMCWEB_LOG_ERROR("Something went wrong while querying dbus!");
            return;
        }

        BMCWEB_LOG_DEBUG("There were {} satellite configs found at startup",
                         std::to_string(satelliteInfo.size()));
    }

    // Search D-Bus objects for satellite config objects and add their
    // information if valid
    static void findSatelliteConfigs(
        const dbus::utility::ManagedObjectType& objects,
        std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        for (const auto& objectPath : objects)
        {
            for (const auto& interface : objectPath.second)
            {
                if (interface.first ==
                    "xyz.openbmc_project.Configuration.SatelliteController")
                {
                    BMCWEB_LOG_DEBUG("Found Satellite Controller at {}",
                                     objectPath.first.str);

                    if (!satelliteInfo.empty())
                    {
                        BMCWEB_LOG_ERROR(
                            "Redfish Aggregation only supports one satellite!");
                        BMCWEB_LOG_DEBUG("Clearing all satellite data");
                        satelliteInfo.clear();
                        return;
                    }

                    // For now assume there will only be one satellite config.
                    // Assign it the name/prefix "5B247A"
                    addSatelliteConfig("5B247A", interface.second,
                                       satelliteInfo);
                }
            }
        }
    }

    // Parse the properties of a satellite config object and add the
    // configuration if the properties are valid
    static void addSatelliteConfig(
        const std::string& name,
        const dbus::utility::DBusPropertiesMap& properties,
        std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        boost::urls::url url;

        for (const auto& prop : properties)
        {
            if (prop.first == "Hostname")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR("Invalid Hostname value");
                    return;
                }
                url.set_host(*propVal);
            }

            else if (prop.first == "Port")
            {
                const uint64_t* propVal = std::get_if<uint64_t>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR("Invalid Port value");
                    return;
                }

                if (*propVal > std::numeric_limits<uint16_t>::max())
                {
                    BMCWEB_LOG_ERROR("Port value out of range");
                    return;
                }
                url.set_port(std::to_string(static_cast<uint16_t>(*propVal)));
            }

            else if (prop.first == "AuthType")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR("Invalid AuthType value");
                    return;
                }

                // For now assume authentication not required to communicate
                // with the satellite BMC
                if (*propVal != "None")
                {
                    BMCWEB_LOG_ERROR(
                        "Unsupported AuthType value: {}, only \"none\" is supported",
                        *propVal);
                    return;
                }
                url.set_scheme("http");
            }
        } // Finished reading properties

        // Make sure all required config information was made available
        if (url.host().empty())
        {
            BMCWEB_LOG_ERROR("Satellite config {} missing Host", name);
            return;
        }

        if (!url.has_port())
        {
            BMCWEB_LOG_ERROR("Satellite config {} missing Port", name);
            return;
        }

        if (!url.has_scheme())
        {
            BMCWEB_LOG_ERROR("Satellite config {} missing AuthType", name);
            return;
        }

        std::string resultString;
        auto result = satelliteInfo.insert_or_assign(name, std::move(url));
        if (result.second)
        {
            resultString = "Added new satellite config ";
        }
        else
        {
            resultString = "Updated existing satellite config ";
        }

        BMCWEB_LOG_DEBUG("{}{} at {}://{}", resultString, name,
                         result.first->second.scheme(),
                         result.first->second.encoded_host_and_port());
    }

    enum AggregationType
    {
        Collection,
        ContainsSubordinate,
        Resource,
    };

    static void
        startAggregation(AggregationType aggType, const crow::Request& thisReq,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        if (thisReq.method() != boost::beast::http::verb::get)
        {
            if (aggType == AggregationType::Collection)
            {
                BMCWEB_LOG_DEBUG(
                    "Only aggregate GET requests to top level collections");
                return;
            }

            if (aggType == AggregationType::ContainsSubordinate)
            {
                BMCWEB_LOG_DEBUG(
                    "Only aggregate GET requests when uptree of a top level collection");
                return;
            }
        }

        // Create a copy of thisReq so we we can still locally process the req
        std::error_code ec;
        auto localReq = std::make_shared<crow::Request>(thisReq.req, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Failed to create copy of request");
            if (aggType == AggregationType::Resource)
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        getSatelliteConfigs(
            std::bind_front(aggregateAndHandle, aggType, localReq, asyncResp));
    }

    static void findSatellite(
        const crow::Request& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo,
        std::string_view memberName)
    {
        // Determine if the resource ID begins with a known prefix
        for (const auto& satellite : satelliteInfo)
        {
            std::string targetPrefix = satellite.first;
            targetPrefix += "_";
            if (memberName.starts_with(targetPrefix))
            {
                BMCWEB_LOG_DEBUG("\"{}\" is a known prefix", satellite.first);

                // Remove the known prefix from the request's URI and
                // then forward to the associated satellite BMC
                getInstance().forwardRequest(req, asyncResp, satellite.first,
                                             satelliteInfo);
                return;
            }
        }

        // We didn't recognize the prefix and need to return a 404
        std::string nameStr = req.url().segments().back();
        messages::resourceNotFound(asyncResp->res, "", nameStr);
    }

    // Intended to handle an incoming request based on if Redfish Aggregation
    // is enabled.  Forwards request to satellite BMC if it exists.
    static void aggregateAndHandle(
        AggregationType aggType,
        const std::shared_ptr<crow::Request>& sharedReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const boost::system::error_code& ec,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        if (sharedReq == nullptr)
        {
            return;
        }
        // Something went wrong while querying dbus
        if (ec)
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // No satellite configs means we don't need to keep attempting to
        // aggregate
        if (satelliteInfo.empty())
        {
            // For collections or resources that can contain a subordinate
            // top level collection we'll also handle the request locally so we
            // don't need to write an error code
            if (aggType == AggregationType::Resource)
            {
                std::string nameStr = sharedReq->url().segments().back();
                messages::resourceNotFound(asyncResp->res, "", nameStr);
            }
            return;
        }

        const crow::Request& thisReq = *sharedReq;
        BMCWEB_LOG_DEBUG("Aggregation is enabled, begin processing of {}",
                         thisReq.target());

        // We previously determined the request is for a collection.  No need to
        // check again
        if (aggType == AggregationType::Collection)
        {
            BMCWEB_LOG_DEBUG("Aggregating a collection");
            // We need to use a specific response handler and send the
            // request to all known satellites
            getInstance().forwardCollectionRequests(thisReq, asyncResp,
                                                    satelliteInfo);
            return;
        }

        // We previously determined the request may contain a subordinate
        // collection.  No need to check again
        if (aggType == AggregationType::ContainsSubordinate)
        {
            BMCWEB_LOG_DEBUG(
                "Aggregating what may have a subordinate collection");
            // We need to use a specific response handler and send the
            // request to all known satellites
            getInstance().forwardContainsSubordinateRequests(thisReq, asyncResp,
                                                             satelliteInfo);
            return;
        }

        const boost::urls::segments_view urlSegments = thisReq.url().segments();
        boost::urls::url currentUrl("/");
        boost::urls::segments_view::const_iterator it = urlSegments.begin();
        boost::urls::segments_view::const_iterator end = urlSegments.end();

        // Skip past the leading "/redfish/v1"
        it++;
        it++;
        for (; it != end; it++)
        {
            if (std::binary_search(topCollections.begin(), topCollections.end(),
                                   currentUrl.buffer()))
            {
                // We've matched a resource collection so this current segment
                // must contain an aggregation prefix
                findSatellite(thisReq, asyncResp, satelliteInfo, *it);
                return;
            }

            currentUrl.segments().push_back(*it);
        }

        // We shouldn't reach this point since we should've hit one of the
        // previous exits
        messages::internalError(asyncResp->res);
    }

    // Attempt to forward a request to the satellite BMC associated with the
    // prefix.
    void forwardRequest(
        const crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::string& prefix,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        const auto& sat = satelliteInfo.find(prefix);
        if (sat == satelliteInfo.end())
        {
            // Realistically this shouldn't get called since we perform an
            // earlier check to make sure the prefix exists
            BMCWEB_LOG_ERROR("Unrecognized satellite prefix \"{}\"", prefix);
            return;
        }

        // We need to strip the prefix from the request's path
        boost::urls::url targetURI(thisReq.target());
        std::string path = thisReq.url().path();
        size_t pos = path.find(prefix + "_");
        if (pos == std::string::npos)
        {
            // If this fails then something went wrong
            BMCWEB_LOG_ERROR("Error removing prefix \"{}_\" from request URI",
                             prefix);
            messages::internalError(asyncResp->res);
            return;
        }
        path.erase(pos, prefix.size() + 1);

        std::function<void(crow::Response&)> cb =
            std::bind_front(processResponse, prefix, asyncResp);

        std::string data = thisReq.body();
        boost::urls::url url(sat->second);
        url.set_path(path);
        if (targetURI.has_query())
        {
            url.set_query(targetURI.query());
        }
        client.sendDataWithCallback(std::move(data), url,
                                    ensuressl::VerifyCertificate::Verify,
                                    thisReq.fields(), thisReq.method(), cb);
    }

    // Forward a request for a collection URI to each known satellite BMC
    void forwardCollectionRequests(
        const crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        for (const auto& sat : satelliteInfo)
        {
            std::function<void(crow::Response&)> cb = std::bind_front(
                processCollectionResponse, sat.first, asyncResp);

            boost::urls::url url(sat.second);
            url.set_path(thisReq.url().path());
            if (thisReq.url().has_query())
            {
                url.set_query(thisReq.url().query());
            }
            std::string data = thisReq.body();
            client.sendDataWithCallback(std::move(data), url,
                                        ensuressl::VerifyCertificate::Verify,
                                        thisReq.fields(), thisReq.method(), cb);
        }
    }

    // Forward request for a URI that is uptree of a top level collection to
    // each known satellite BMC
    void forwardContainsSubordinateRequests(
        const crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        for (const auto& sat : satelliteInfo)
        {
            std::function<void(crow::Response&)> cb = std::bind_front(
                processContainsSubordinateResponse, sat.first, asyncResp);

            // will ignore an expanded resource in the response if that resource
            // is not already supported by the aggregating BMC
            // TODO: Improve the processing so that we don't have to strip query
            // params in this specific case
            boost::urls::url url(sat.second);
            url.set_path(thisReq.url().path());

            std::string data = thisReq.body();

            client.sendDataWithCallback(std::move(data), url,
                                        ensuressl::VerifyCertificate::Verify,
                                        thisReq.fields(), thisReq.method(), cb);
        }
    }

  public:
    explicit RedfishAggregator(boost::asio::io_context& ioc) :
        client(ioc,
               std::make_shared<crow::ConnectionPolicy>(getAggregationPolicy()))
    {
        getSatelliteConfigs(constructorCallback);
    }
    RedfishAggregator(const RedfishAggregator&) = delete;
    RedfishAggregator& operator=(const RedfishAggregator&) = delete;
    RedfishAggregator(RedfishAggregator&&) = delete;
    RedfishAggregator& operator=(RedfishAggregator&&) = delete;
    ~RedfishAggregator() = default;

    static RedfishAggregator& getInstance(boost::asio::io_context* io = nullptr)
    {
        static RedfishAggregator handler(*io);
        return handler;
    }

    // Polls D-Bus to get all available satellite config information
    // Expects a handler which interacts with the returned configs
    static void getSatelliteConfigs(
        std::function<
            void(const boost::system::error_code&,
                 const std::unordered_map<std::string, boost::urls::url>&)>
            handler)
    {
        BMCWEB_LOG_DEBUG("Gathering satellite configs");
        sdbusplus::message::object_path path("/xyz/openbmc_project/inventory");
        dbus::utility::getManagedObjects(
            "xyz.openbmc_project.EntityManager", path,
            [handler{std::move(handler)}](
                const boost::system::error_code& ec,
                const dbus::utility::ManagedObjectType& objects) {
            std::unordered_map<std::string, boost::urls::url> satelliteInfo;
            if (ec)
            {
                BMCWEB_LOG_ERROR("DBUS response error {}, {}", ec.value(),
                                 ec.message());
                handler(ec, satelliteInfo);
                return;
            }

            // Maps a chosen alias representing a satellite BMC to a url
            // containing the information required to create a http
            // connection to the satellite
            findSatelliteConfigs(objects, satelliteInfo);

            if (!satelliteInfo.empty())
            {
                BMCWEB_LOG_DEBUG(
                    "Redfish Aggregation enabled with {} satellite BMCs",
                    std::to_string(satelliteInfo.size()));
            }
            else
            {
                BMCWEB_LOG_DEBUG(
                    "No satellite BMCs detected.  Redfish Aggregation not enabled");
            }
            handler(ec, satelliteInfo);
        });
    }

    // Processes the response returned by a satellite BMC and loads its
    // contents into asyncResp
    static void
        processResponse(std::string_view prefix,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        crow::Response& resp)
    {
        // 429 and 502 mean we didn't actually send the request so don't
        // overwrite the response headers in that case
        if ((resp.result() == boost::beast::http::status::too_many_requests) ||
            (resp.result() == boost::beast::http::status::bad_gateway))
        {
            asyncResp->res.result(resp.result());
            return;
        }

        // We want to attempt prefix fixing regardless of response code
        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if (isJsonContentType(resp.getHeaderValue("Content-Type")))
        {
            nlohmann::json jsonVal = nlohmann::json::parse(*resp.body(),
                                                           nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR("Error parsing satellite response as JSON");
                messages::operationFailed(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG("Successfully parsed satellite response");

            addPrefixes(jsonVal, prefix);

            BMCWEB_LOG_DEBUG("Added prefix to parsed satellite response");

            asyncResp->res.result(resp.result());
            asyncResp->res.jsonValue = std::move(jsonVal);

            BMCWEB_LOG_DEBUG("Finished writing asyncResp");
        }
        else
        {
            // We allow any Content-Type that is not "application/json" now
            asyncResp->res.result(resp.result());
            asyncResp->res.copyBody(resp);
        }
        addAggregatedHeaders(asyncResp->res, resp, prefix);
    }

    // Processes the collection response returned by a satellite BMC and merges
    // its "@odata.id" values
    static void processCollectionResponse(
        const std::string& prefix,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        crow::Response& resp)
    {
        // 429 and 502 mean we didn't actually send the request so don't
        // overwrite the response headers in that case
        if ((resp.result() == boost::beast::http::status::too_many_requests) ||
            (resp.result() == boost::beast::http::status::bad_gateway))
        {
            return;
        }

        if (resp.resultInt() != 200)
        {
            BMCWEB_LOG_DEBUG(
                "Collection resource does not exist in satellite BMC \"{}\"",
                prefix);
            // Return the error if we haven't had any successes
            if (asyncResp->res.resultInt() != 200)
            {
                asyncResp->res.result(resp.result());
                asyncResp->res.copyBody(resp);
            }
            return;
        }

        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if (isJsonContentType(resp.getHeaderValue("Content-Type")))
        {
            nlohmann::json jsonVal = nlohmann::json::parse(*resp.body(),
                                                           nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR("Error parsing satellite response as JSON");

                // Notify the user if doing so won't overwrite a valid response
                if (asyncResp->res.resultInt() != 200)
                {
                    messages::operationFailed(asyncResp->res);
                }
                return;
            }

            BMCWEB_LOG_DEBUG("Successfully parsed satellite response");

            // Now we need to add the prefix to the URIs contained in the
            // response.
            addPrefixes(jsonVal, prefix);

            BMCWEB_LOG_DEBUG("Added prefix to parsed satellite response");

            // If this resource collection does not exist on the aggregating bmc
            // and has not already been added from processing the response from
            // a different satellite then we need to completely overwrite
            // asyncResp
            if (asyncResp->res.resultInt() != 200)
            {
                // We only want to aggregate collections that contain a
                // "Members" array
                if ((!jsonVal.contains("Members")) &&
                    (!jsonVal["Members"].is_array()))
                {
                    BMCWEB_LOG_DEBUG(
                        "Skipping aggregating unsupported resource");
                    return;
                }

                BMCWEB_LOG_DEBUG(
                    "Collection does not exist, overwriting asyncResp");
                asyncResp->res.result(resp.result());
                asyncResp->res.jsonValue = std::move(jsonVal);
                asyncResp->res.addHeader("Content-Type", "application/json");

                BMCWEB_LOG_DEBUG("Finished overwriting asyncResp");
            }
            else
            {
                // We only want to aggregate collections that contain a
                // "Members" array
                if ((!asyncResp->res.jsonValue.contains("Members")) &&
                    (!asyncResp->res.jsonValue["Members"].is_array()))

                {
                    BMCWEB_LOG_DEBUG(
                        "Skipping aggregating unsupported resource");
                    return;
                }

                BMCWEB_LOG_DEBUG(
                    "Adding aggregated resources from \"{}\" to collection",
                    prefix);

                // TODO: This is a potential race condition with multiple
                // satellites and the aggregating bmc attempting to write to
                // update this array.  May need to cascade calls to the next
                // satellite at the end of this function.
                // This is presumably not a concern when there is only a single
                // satellite since the aggregating bmc should have completed
                // before the response is received from the satellite.

                auto& members = asyncResp->res.jsonValue["Members"];
                auto& satMembers = jsonVal["Members"];
                for (auto& satMem : satMembers)
                {
                    members.emplace_back(std::move(satMem));
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();

                // TODO: Do we need to sort() after updating the array?
            }
        }
        else
        {
            BMCWEB_LOG_ERROR("Received unparsable response from \"{}\"",
                             prefix);
            // We received a response that was not a json.
            // Notify the user only if we did not receive any valid responses
            // and if the resource collection does not already exist on the
            // aggregating BMC
            if (asyncResp->res.resultInt() != 200)
            {
                messages::operationFailed(asyncResp->res);
            }
        }
    } // End processCollectionResponse()

    // Processes the response returned by a satellite BMC and merges any
    // properties whose "@odata.id" value is the URI or either a top level
    // collection or is uptree from a top level collection
    static void processContainsSubordinateResponse(
        const std::string& prefix,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        crow::Response& resp)
    {
        // 429 and 502 mean we didn't actually send the request so don't
        // overwrite the response headers in that case
        if ((resp.result() == boost::beast::http::status::too_many_requests) ||
            (resp.result() == boost::beast::http::status::bad_gateway))
        {
            return;
        }

        if (resp.resultInt() != 200)
        {
            BMCWEB_LOG_DEBUG(
                "Resource uptree from Collection does not exist in satellite BMC \"{}\"",
                prefix);
            // Return the error if we haven't had any successes
            if (asyncResp->res.resultInt() != 200)
            {
                asyncResp->res.result(resp.result());
                asyncResp->res.copyBody(resp);
            }
            return;
        }

        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if (isJsonContentType(resp.getHeaderValue("Content-Type")))
        {
            bool addedLinks = false;
            nlohmann::json jsonVal = nlohmann::json::parse(*resp.body(),
                                                           nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR("Error parsing satellite response as JSON");

                // Notify the user if doing so won't overwrite a valid response
                if (asyncResp->res.resultInt() != 200)
                {
                    messages::operationFailed(asyncResp->res);
                }
                return;
            }

            BMCWEB_LOG_DEBUG("Successfully parsed satellite response");

            // Parse response and add properties missing from the AsyncResp
            // Valid properties will be of the form <property>.@odata.id and
            // @odata.id is a <URI>.  In other words, the json should contain
            // multiple properties such that
            // {"<property>":{"@odata.id": "<URI>"}}
            nlohmann::json::object_t* object =
                jsonVal.get_ptr<nlohmann::json::object_t*>();
            if (object == nullptr)
            {
                BMCWEB_LOG_ERROR("Parsed JSON was not an object?");
                return;
            }

            for (std::pair<const std::string, nlohmann::json>& prop : *object)
            {
                if (!prop.second.contains("@odata.id"))
                {
                    continue;
                }

                std::string* strValue =
                    prop.second["@odata.id"].get_ptr<std::string*>();
                if (strValue == nullptr)
                {
                    BMCWEB_LOG_CRITICAL("Field wasn't a string????");
                    continue;
                }
                if (!searchCollectionsArray(*strValue, SearchType::CollOrCon))
                {
                    continue;
                }

                addedLinks = true;
                if (!asyncResp->res.jsonValue.contains(prop.first))
                {
                    // Only add the property if it did not already exist
                    BMCWEB_LOG_DEBUG("Adding link for {} from BMC {}",
                                     *strValue, prefix);
                    asyncResp->res.jsonValue[prop.first]["@odata.id"] =
                        *strValue;
                    continue;
                }
            }

            // If we added links to a previously unsuccessful (non-200) response
            // then we need to make sure the response contains the bare minimum
            // amount of additional information that we'd expect to have been
            // populated.
            if (addedLinks && (asyncResp->res.resultInt() != 200))
            {
                // This resource didn't locally exist or an error
                // occurred while generating the response.  Remove any
                // error messages and update the error code.
                asyncResp->res.jsonValue.erase(
                    asyncResp->res.jsonValue.find("error"));
                asyncResp->res.result(resp.result());

                const auto& it1 = object->find("@odata.id");
                if (it1 != object->end())
                {
                    asyncResp->res.jsonValue["@odata.id"] = (it1->second);
                }
                const auto& it2 = object->find("@odata.type");
                if (it2 != object->end())
                {
                    asyncResp->res.jsonValue["@odata.type"] = (it2->second);
                }
                const auto& it3 = object->find("Id");
                if (it3 != object->end())
                {
                    asyncResp->res.jsonValue["Id"] = (it3->second);
                }
                const auto& it4 = object->find("Name");
                if (it4 != object->end())
                {
                    asyncResp->res.jsonValue["Name"] = (it4->second);
                }
            }
        }
        else
        {
            BMCWEB_LOG_ERROR("Received unparsable response from \"{}\"",
                             prefix);
            // We received as response that was not a json
            // Notify the user only if we did not receive any valid responses,
            // and if the resource does not already exist on the aggregating BMC
            if (asyncResp->res.resultInt() != 200)
            {
                messages::operationFailed(asyncResp->res);
            }
        }
    }

    // Entry point to Redfish Aggregation
    // Returns Result stating whether or not we still need to locally handle the
    // request
    static Result
        beginAggregation(const crow::Request& thisReq,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        using crow::utility::OrMorePaths;
        using crow::utility::readUrlSegments;
        boost::urls::url_view url = thisReq.url();

        // We don't need to aggregate JsonSchemas due to potential issues such
        // as version mismatches between aggregator and satellite BMCs.  For
        // now assume that the aggregator has all the schemas and versions that
        // the aggregated server has.
        if (crow::utility::readUrlSegments(url, "redfish", "v1", "JsonSchemas",
                                           crow::utility::OrMorePaths()))
        {
            return Result::LocalHandle;
        }

        // The first two segments should be "/redfish/v1".  We need to check
        // that before we can search topCollections
        if (!crow::utility::readUrlSegments(url, "redfish", "v1",
                                            crow::utility::OrMorePaths()))
        {
            return Result::LocalHandle;
        }

        // Parse the URI to see if it begins with a known top level collection
        // such as:
        // /redfish/v1/Chassis
        // /redfish/v1/UpdateService/FirmwareInventory
        const boost::urls::segments_view urlSegments = url.segments();
        boost::urls::url currentUrl("/");
        boost::urls::segments_view::const_iterator it = urlSegments.begin();
        boost::urls::segments_view::const_iterator end = urlSegments.end();

        // Skip past the leading "/redfish/v1"
        it++;
        it++;
        for (; it != end; it++)
        {
            const std::string& collectionItem = *it;
            if (std::binary_search(topCollections.begin(), topCollections.end(),
                                   currentUrl.buffer()))
            {
                // We've matched a resource collection so this current segment
                // might contain an aggregation prefix
                // TODO: This needs to be rethought when we can support multiple
                // satellites due to
                // /redfish/v1/AggregationService/AggregationSources/5B247A
                // being a local resource describing the satellite
                if (collectionItem.starts_with("5B247A_"))
                {
                    BMCWEB_LOG_DEBUG("Need to forward a request");

                    // Extract the prefix from the request's URI, retrieve the
                    // associated satellite config information, and then forward
                    // the request to that satellite.
                    startAggregation(AggregationType::Resource, thisReq,
                                     asyncResp);
                    return Result::NoLocalHandle;
                }

                // Handle collection URI with a trailing backslash
                // e.g. /redfish/v1/Chassis/
                it++;
                if ((it == end) && collectionItem.empty())
                {
                    startAggregation(AggregationType::Collection, thisReq,
                                     asyncResp);
                }

                // We didn't recognize the prefix or it's a collection with a
                // trailing "/".  In both cases we still want to locally handle
                // the request
                return Result::LocalHandle;
            }

            currentUrl.segments().push_back(collectionItem);
        }

        // If we made it here then currentUrl could contain a top level
        // collection URI without a trailing "/", e.g. /redfish/v1/Chassis
        if (std::binary_search(topCollections.begin(), topCollections.end(),
                               currentUrl.buffer()))
        {
            startAggregation(AggregationType::Collection, thisReq, asyncResp);
            return Result::LocalHandle;
        }

        // If nothing else then the request could be for a resource which has a
        // top level collection as a subordinate
        if (searchCollectionsArray(url.path(), SearchType::ContainsSubordinate))
        {
            startAggregation(AggregationType::ContainsSubordinate, thisReq,
                             asyncResp);
            return Result::LocalHandle;
        }

        BMCWEB_LOG_DEBUG("Aggregation not required for {}", url.buffer());
        return Result::LocalHandle;
    }
};

} // namespace redfish
