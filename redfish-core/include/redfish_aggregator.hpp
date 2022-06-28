#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <http_connection.hpp>

namespace redfish
{

// Search the json for all "@odata.id" keys and add the prefix to the URIs
// which are their associated values.
static void addPrefixes(nlohmann::json& json, const std::string& prefix)
{
    for (auto& item : json.items())
    {
        // URIs we need to fix will appear as the value for "@odata.id" keys
        if (item.key() == "@odata.id")
        {
            std::string odataVal = json["@odata.id"];

            // Make sure the URI is properly formatted
            auto parsed = boost::urls::parse_relative_ref(odataVal);
            if (!parsed)
            {
                // Even if one fails, we should still try to fix as many as
                // we can
                BMCWEB_LOG_ERROR << "Unable to parse path: " << odataVal;
                continue;
            }

            boost::urls::url thisURL(*parsed);
            auto segments = thisURL.segments();

            // We don't need to add prefixes to these URIs since
            // /redfish/v1/UpdateService/ itself is not a collection
            // /redfish/v1/UpdateService/FirmwareInventory
            // /redfish/v1/UpdateService/SoftwareInventory
            if (((segments.size() == 5) && (segments[4].empty())) ||
                ((segments.size() == 4) && (!segments[3].empty())))
            {
                std::string seg2(segments[2]);
                std::string seg3(segments[3]);
                if ((seg2 == "UpdateService") &&
                    ((seg3 == "FirmwareInventory") ||
                     (seg3 == "SoftwareInventory")))
                {
                    BMCWEB_LOG_DEBUG
                        << "Skipping UpdateService URI prefix fixing";
                    continue;
                }
            }

            // A collection URI that ends with "/" such as
            // "/redfish/v1/Chassis/" will have 4 segments so we need to make
            // sure we don't try to add a prefix to an empty segment
            if ((segments.size() >= 4) && (!segments[3].empty()))
            {
                // The "+ 4" is to account for each "/" in the path
                size_t offset = segments[0].size() + segments[1].size() +
                                segments[2].size() + 4;

                // We also need to aggregate FirmwareInventory and
                // SoftwareInventory so add an extra offset
                // /redfish/v1/UpdateService/FirmwareInventory/<id>
                // /redfish/v1/UpdateService/SoftwareInventory/<id>
                if ((segments.size() >= 5) && (!segments[4].empty()))
                {
                    std::string seg2(segments[2]);
                    std::string seg3(segments[3]);
                    if ((seg2 == "UpdateService") &&
                        ((seg3 == "FirmwareInventory") ||
                         (seg3 == "SoftwareInventory")))
                    {
                        BMCWEB_LOG_DEBUG
                            << "Fixing up aggregated URI under UpdateService";
                        offset += segments[3].size() + 1;
                    }
                }

                odataVal.insert(offset, prefix + "_");
                json["@odata.id"] = std::move(odataVal);
            }
        }
        else if (item.value().is_structured())
        {
            // Keep parsing the rest of the json
            addPrefixes(item.value(), prefix);
        }
    }
}

class RedfishAggregator
{
  private:
    const std::string retryPolicyName = "RedfishAggregation";
    const uint32_t retryAttempts = 5;
    const uint32_t retryTimeoutInterval = 0;
    const std::string id = "Aggregator";

    RedfishAggregator()
    {
        getSatelliteConfigs(constructorCallback);

        // Setup the retry policy to be used by Redfish Aggregation
        crow::HttpClient::getInstance().setRetryConfig(
            retryAttempts, retryTimeoutInterval, aggregationRetryHandler,
            retryPolicyName);
    }

    static inline boost::system::error_code
        aggregationRetryHandler(unsigned int respCode)
    {
        // As a default, assume 200X is alright.
        // We don't need to retry on a 404
        if ((respCode < 200) || ((respCode >= 300) && (respCode != 404)))
        {
            return boost::system::errc::make_error_code(
                boost::system::errc::result_out_of_range);
        }

        // Return 0 if the response code is valid
        return boost::system::errc::make_error_code(
            boost::system::errc::success);
    }

    // Dummy callback used by the Constructor so that it can report the number
    // of satellite configs when the class is first created
    static void constructorCallback(
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        BMCWEB_LOG_DEBUG << "There were "
                         << std::to_string(satelliteInfo.size())
                         << " satellite configs found at startup";
    }

    // Polls D-Bus to get all available satellite config information
    // Expects a handler which interacts with the returned configs
    static void getSatelliteConfigs(
        const std::function<void(
            const std::unordered_map<std::string, boost::urls::url>&)>& handler)
    {
        BMCWEB_LOG_DEBUG << "Gathering satellite configs";
        crow::connections::systemBus->async_method_call(
            [handler](const boost::system::error_code ec,
                      const dbus::utility::ManagedObjectType& objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "DBUS response error " << ec.value() << ", "
                                 << ec.message();
                return;
            }

            // Maps a chosen alias representing a satellite BMC to a url
            // containing the information required to create a http
            // connection to the satellite
            std::unordered_map<std::string, boost::urls::url> satelliteInfo;

            findSatelliteConfigs(objects, satelliteInfo);

            if (!satelliteInfo.empty())
            {
                BMCWEB_LOG_DEBUG << "Redfish Aggregation enabled with "
                                 << std::to_string(satelliteInfo.size())
                                 << " satellite BMCs";
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "No satellite BMCs detected.  Redfish Aggregation not enabled";
            }
            handler(satelliteInfo);
            },
            "xyz.openbmc_project.EntityManager", "/",
            "org.freedesktop.DBus.ObjectManager", "GetManagedObjects");
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
                    BMCWEB_LOG_DEBUG << "Found Satellite Controller at "
                                     << objectPath.first.str;

                    addSatelliteConfig(interface.second, satelliteInfo);
                }
            }
        }
    }

    // Parse the properties of a satellite config object and add the
    // configuration if the properties are valid
    static void addSatelliteConfig(
        const dbus::utility::DBusPropertiesMap& properties,
        std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        boost::urls::url url;
        std::string name;

        for (const auto& prop : properties)
        {
            if (prop.first == "Name")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Name value";
                    return;
                }

                // The IDs will become <Name>_<ID> so the name should not
                // contain a '_'
                if (propVal->find('_') != std::string::npos)
                {
                    BMCWEB_LOG_ERROR << "Name cannot contain a \"_\"";
                    return;
                }
                name = *propVal;
            }

            else if (prop.first == "Hostname")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Hostname value";
                    return;
                }
                url.set_host(*propVal);
            }

            else if (prop.first == "Port")
            {
                const uint64_t* propVal = std::get_if<uint64_t>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid Port value";
                    return;
                }

                if (*propVal > std::numeric_limits<uint16_t>::max())
                {
                    BMCWEB_LOG_ERROR << "Port value out of range";
                    return;
                }
                url.set_port(static_cast<uint16_t>(*propVal));
            }

            else if (prop.first == "AuthType")
            {
                const std::string* propVal =
                    std::get_if<std::string>(&prop.second);
                if (propVal == nullptr)
                {
                    BMCWEB_LOG_ERROR << "Invalid AuthType value";
                    return;
                }

                // For now assume authentication not required to communicate
                // with the satellite BMC
                if (*propVal != "None")
                {
                    BMCWEB_LOG_ERROR
                        << "Unsupported AuthType value: " << *propVal
                        << ", only \"none\" is supported";
                    return;
                }
                url.set_scheme("http");
            }
        } // Finished reading properties

        // Make sure all required config information was made available
        if (name.empty())
        {
            BMCWEB_LOG_ERROR << "Satellite config missing Name";
            return;
        }

        if (url.host().empty())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name << " missing Host";
            return;
        }

        if (!url.has_port())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name << " missing Port";
            return;
        }

        if (!url.has_scheme())
        {
            BMCWEB_LOG_ERROR << "Satellite config " << name
                             << " missing AuthType";
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

        BMCWEB_LOG_DEBUG << resultString << name << " at "
                         << result.first->second.scheme() << "://"
                         << result.first->second.encoded_host_and_port();
    }

    // Intended to handle an incoming request based on if Redfish Aggregation
    // is enabled.  Forwards request to satellite BMC if it exists.
    void aggregateAndHandle(
        crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        // satelliteInfo will contain all satellite config information
        // If it is empty then that means we don't need to perform any
        // aggregation operations
        if (!satelliteInfo.empty())
        {
            BMCWEB_LOG_DEBUG << "Aggregation is enabled, begin processing of "
                             << thisReq.target();

            std::vector<std::string> prefixes;
            getPrefixes(prefixes, satelliteInfo);
            boost::urls::string_view origTarget(thisReq.target().data(),
                                                thisReq.target().size());

            // We need to forward the request if its path begins with form of
            // /redfish/v1/<resource>/<known_prefix>_<resource_id>
            auto parsed = boost::urls::parse_relative_ref(origTarget);
            if (!parsed)
            {
                BMCWEB_LOG_ERROR << "Unable to parse path";
                return;
            }

            boost::urls::url thisURL(*parsed);
            auto segments = thisURL.segments();

            // TODO: We will also want to make sure that satellite only
            // collections will show up in the service root /redfish/v1

            // Is the request for a resource collection?:
            // /redfish/v1/<resource>/
            // /redfish/v1/<resource>
            if (((segments.size() == 4) && (segments[3].empty())) ||
                ((segments.size() == 3) && (!segments[2].empty())))
            {
                // /redfish/v1/UpdateService should not be forwarded
                std::string seg2(segments[2]);
                if (seg2 != "UpdateService")
                {
                    // We need to use a specific response handler and send the
                    // request to all known satellites
                    forwardCollectionRequests(thisReq, asyncResp,
                                              satelliteInfo);
                    return;
                }
            }

            // We also need to aggregate FirmwareInventory and SoftwareInventory
            // as collections
            // /redfish/v1/UpdateService/FirmwareInventory
            // /redfish/v1/UpdateService/SoftwareInventory
            if (((segments.size() == 5) && (segments[4].empty())) ||
                ((segments.size() == 4) && (!segments[3].empty())))
            {
                std::string seg2(segments[2]);
                if ((seg2 == "UpdateService") &&
                    ((seg3 == "FirmwareInventory") ||
                     (seg3 == "SoftwareInventory")))
                {
                    BMCWEB_LOG_DEBUG
                        << "Forwarding a collection under UpdateService";
                    // We need to use a specific response handler and send the
                    // request to all known satellites
                    forwardCollectionRequests(thisReq, asyncResp,
                                              satelliteInfo);
                    return;
                }
            }

            if (segments.size() >= 4)
            {
                std::string targetURI(thisURL.encoded_path());

                // Determine if the resource ID begins with a known prefix
                for (const auto& prefix : prefixes)
                {
                    size_t pos = targetURI.find(prefix + "_");
                    if (pos != std::string::npos)
                    {
                        BMCWEB_LOG_DEBUG << "\"" << prefix
                                         << "\" is a known prefix";

                        // Remove the known prefix from the request's URI and
                        // then forward to the associated satellite BMC
                        forwardRequest(thisReq, asyncResp, prefix,
                                       satelliteInfo);
                        return;
                    }
                }
            }

            // We didn't recognize the request as one that should be aggregated
            // so we'll just locally handle it.  No need to attempt to forward
            // anything
            BMCWEB_LOG_DEBUG << "Aggregation not required";
        } // End Redfish Aggregation initial processing
    }     // End aggregateAndHandle()

    // Returns vector of all aggregation prefixes
    static void getPrefixes(
        std::vector<std::string>& prefixes,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        prefixes.clear();
        if (satelliteInfo.empty())
        {
            return;
        }

        for (const auto& sat : satelliteInfo)
        {
            BMCWEB_LOG_DEBUG << "Found aggregation prefix \"" << sat.first
                             << "\"";
            prefixes.push_back(sat.first);
        }

        BMCWEB_LOG_DEBUG << "Total aggregation prefixes: " << prefixes.size();
    }

    // Attempt to forward a request to the satellite BMC associated with the
    // prefix.
    void forwardRequest(
        crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::string& prefix,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        const auto& sat = satelliteInfo.find(prefix);
        if (sat == satelliteInfo.end())
        {
            // Realistically this shouldn't get called since we perform an
            // earlier check to make sure the prefix exists
            BMCWEB_LOG_ERROR << "Unrecognized satellite prefix \"" << prefix
                             << "\"";
            return;
        }

        // We need to strip the prefix from the request's path
        std::string targetURI(thisReq.target());
        size_t pos = targetURI.find(prefix + "_");
        if (pos == std::string::npos)
        {
            // If this fails then something went wrong
            BMCWEB_LOG_ERROR << "Error removing prefix \"" << prefix
                             << "_\" from request URI";
            messages::internalError(asyncResp->res);
            return;
        }

        targetURI.erase(pos, prefix.size() + 1);

        std::function<void(crow::Response&)> cb =
            std::bind_front(processResponse, prefix, asyncResp);

        std::string data = boost::lexical_cast<std::string>(thisReq.req.body());
        crow::HttpClient::getInstance().sendDataWithCallback(
            data, id, std::string(sat->second.host()),
            sat->second.port_number(), targetURI, thisReq.fields,
            thisReq.method(), retryPolicyName, cb);
    }

    // Forward a request for a collection URI to each known satellite BMC
    void forwardCollectionRequests(
        crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        for (const auto& sat : satelliteInfo)
        {
            std::function<void(crow::Response&)> cb = std::bind_front(
                processCollectionResponse, sat.first, asyncResp);

            std::string targetURI(thisReq.target());
            std::string data =
                boost::lexical_cast<std::string>(thisReq.req.body());
            crow::HttpClient::getInstance().sendDataWithCallback(
                data, id, std::string(sat.second.host()),
                sat.second.port_number(), targetURI, thisReq.fields,
                thisReq.method(), retryPolicyName, cb);
        }
    }

    // Processes the response returned by a satellite BMC and loads its
    // contents into asyncResp
    static void
        processResponse(const std::string& prefix,
                        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        crow::Response& resp)
    {
        // No processing needed if the request wasn't successful
        if (resp.resultInt() != 200)
        {
            BMCWEB_LOG_DEBUG << "No need to parse satellite response";
            asyncResp->res.stringResponse = std::move(resp.stringResponse);
            return;
        }

        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if ((resp.stringResponse->base()["Content-Type"] ==
             "application/json") ||
            (nlohmann::json::accept(resp.body())))
        {
            nlohmann::json jsonVal =
                nlohmann::json::parse(resp.body(), nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing satellite response as JSON";
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Successfully parsed satellite response";

            // Now we need to add the prefix to the URIs contained in the
            // response.
            addPrefixes(jsonVal, prefix);

            BMCWEB_LOG_DEBUG << "Added prefix to parsed satellite response";

            // The aggregating BMC should have also handled this request and
            // returned a 404.  We need to overwrite that.
            asyncResp->res.stringResponse.emplace(
                boost::beast::http::response<
                    boost::beast::http::string_body>{});
            asyncResp->res.result(resp.result());
            asyncResp->res.jsonValue = std::move(jsonVal);

            BMCWEB_LOG_DEBUG << "Finished overwriting asyncResp";
        }
        else
        {
            // We received a 200 response without a parsable payload so just
            // copy it as is
            asyncResp->res.stringResponse = std::move(resp.stringResponse);
        }
    }

    // Processes the collection response returned by a satellite BMC and merges
    // its "@odata.id" values
    static void processCollectionResponse(
        const std::string& prefix,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        crow::Response& resp)
    {
        if (resp.resultInt() != 200)
        {
            BMCWEB_LOG_DEBUG
                << "Collection resource does not exist in satellite BMC \""
                << prefix << "\"";
            return;
        }

        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if ((resp.stringResponse->base()["Content-Type"] ==
             "application/json") ||
            (nlohmann::json::accept(resp.body())))
        {
            nlohmann::json jsonVal =
                nlohmann::json::parse(resp.body(), nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing satellite response as JSON";
                return;
            }

            BMCWEB_LOG_DEBUG << "Successfully parsed satellite response";

            // Now we need to add the prefix to the URIs contained in the
            // response.
            addPrefixes(jsonVal, prefix);

            BMCWEB_LOG_DEBUG << "Added prefix to parsed satellite response";

            // If this resource collection does not exist on the aggregating bmc
            // and has not already been added from processing the response from
            // a different satellite then we need to completely overwrite
            // asyncResp
            if (asyncResp->res.resultInt() != 200)
            {
                // We only want to aggregate collections that contain a
                // "Members" array
                if ((!jsonVal.contains("Members")) ||
                    (!jsonVal["Members"].is_array()))
                {
                    BMCWEB_LOG_DEBUG
                        << "Skipping aggregating unsupported resource";
                    return;
                }

                BMCWEB_LOG_DEBUG
                    << "Collection does not exist, overwriting asyncResp";
                asyncResp->res.stringResponse.emplace(
                    boost::beast::http::response<
                        boost::beast::http::string_body>{});
                asyncResp->res.result(resp.result());
                asyncResp->res.jsonValue = std::move(jsonVal);

                BMCWEB_LOG_DEBUG << "Finished overwriting asyncResp";
            }
            else
            {
                // We only want to aggregate collections that contain a
                // "Members" array
                if ((!asyncResp->res.jsonValue.contains("Members")) ||
                    (!asyncResp->res.jsonValue["Members"].is_array()))

                {
                    BMCWEB_LOG_DEBUG
                        << "Skipping aggregating unsupported resource";
                    return;
                }

                BMCWEB_LOG_DEBUG << "Adding aggregated resources from \""
                                 << prefix << "\" to collection";

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
                    members.push_back(std::move(satMem));
                }
                asyncResp->res.jsonValue["Members@odata.count"] =
                    members.size();

                // TODO: Do we need to sort() after updating the array?
            }
        }
    } // End processCollectionResponse()

  public:
    RedfishAggregator(const RedfishAggregator&) = delete;
    RedfishAggregator& operator=(const RedfishAggregator&) = delete;
    RedfishAggregator(RedfishAggregator&&) = delete;
    RedfishAggregator& operator=(RedfishAggregator&&) = delete;
    ~RedfishAggregator() = default;

    static RedfishAggregator& getInstance()
    {
        static RedfishAggregator handler;
        return handler;
    }

    // Entry point to Redfish Aggregation
    static void
        beginAggregation(const crow::Request& thisReq,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        // Create a copy of thisReq so we we can still locally process the req
        boost::beast::http::request<boost::beast::http::string_body> req =
            thisReq.req;
        std::error_code ec;
        auto localReq = std::make_shared<crow::Request>(req, ec);
        if (ec)
        {
            BMCWEB_LOG_ERROR << "Failed to create copy of request";
            return;
        }

        auto cb = [localReq, asyncResp](
                      const std::unordered_map<std::string, boost::urls::url>&
                          satelliteInfo) {
            getInstance().aggregateAndHandle(*localReq, asyncResp,
                                             satelliteInfo);
        };

        getSatelliteConfigs(std::move(cb));
    }
};
} // namespace redfish
