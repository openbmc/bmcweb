#pragma once

#include <boost/asio/deadline_timer.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <http_connection.hpp>

namespace redfish
{

class RedfishAggregator : public std::enable_shared_from_this<RedfishAggregator>
{
  private:
    const std::string id = "Aggregator";
    const std::string retryPolicyName = "RedfishAggregation";

    RedfishAggregator()
    {
        getSatelliteConfigs(constructorCallback);
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
                    BMCWEB_LOG_ERROR << "DBUS response error " << ec.value()
                                     << ", " << ec.message();
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
    // is enabled.  The provided reqHandler will be called to handle requests
    // meant for URIs associated with the aggregating BMC
    void aggregateAndHandle(
        const std::function<void(crow::Request&,
                                 const std::shared_ptr<bmcweb::AsyncResp>&)>&
            reqHandler,
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

            std::string prefixes;
            getPrefixes(prefixes, satelliteInfo);

            // Make sure the URI is for a Chassis, Managers, Systems, Fabrics,
            // or ComponentIntegrity resource
            // Its form should be /redfish/v1/<valid_resource>/<prefix>_<str>
            const std::regex urlRegex(
                "/redfish/v1/(Chassis|Managers|Systems|Fabrics|ComponentIntegrity)/(" +
                prefixes + ")_.+");

            std::string targetURI(thisReq.target());

            // Is the target URI related to an aggregated resource?
            if (std::regex_match(targetURI, urlRegex))
            {
                // The URI is for an aggregated resource and includes a prefix
                // We need to either remove the prefix so the request can be
                // locally handled, or forward the request to a satellite BMC
                std::vector<std::string> fields;
                boost::split(fields, targetURI, boost::is_any_of("/"));

                // fields[0] will be empty because of the leading "/" in the URI
                // request.  The prefix will therefore be part of fields[4]
                std::vector<std::string> ids;
                boost::split(ids, fields[4], boost::is_any_of("_"));
                BMCWEB_LOG_DEBUG << "Extracted prefix is \"" << ids[0] << "\"";

                // Set the prefix in asyncResp so the links can later be cleaned
                // up
                asyncResp->res.aggregationPrefix = ids[0];

                // Is request meant for the aggregating BMC?
                if (ids[0] == "main")
                {
                    BMCWEB_LOG_DEBUG
                        << "Request received for aggregated native resource";

                    size_t pos = targetURI.find("main_");
                    if (pos == std::string::npos)
                    {
                        // If this happens then something went wrong
                        BMCWEB_LOG_ERROR
                            << "Error removing prefix \"main_\" from request URI";
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // Remove first occurrence of "main_" from the URI so the
                    // aggregating BMC can handle the request correctly
                    targetURI.erase(pos, 5);
                    thisReq.target(std::string_view(targetURI));
                }
                else
                {
                    // We need to search the satellite prefixes";
                    BMCWEB_LOG_DEBUG << "Searching satellite prefixes for "
                                     << ids[0];

                    // Will either forward the request to the associated
                    // satellite BMC or set a 404 if the prefix is not
                    // recognized
                    forwardRequest(thisReq, asyncResp, ids[0], satelliteInfo);

                    // No need to locally handle this request
                    return;
                }
            } // end handling of aggregated resource
            else
            {
                // Make sure the previous check did not fail because a required
                // prefix was not supplied
                const std::regex illegalRegex(
                    "/redfish/v1/(Chassis|Managers|Systems|Fabrics|ComponentIntegrity)/(.+)");

                if (std::regex_match(targetURI, illegalRegex))
                {
                    // It is not possible to have an aggregated resource that
                    // does not include a prefix in the URI
                    asyncResp->res.result(
                        boost::beast::http::status::not_found);
                    return;
                }

                // If this is actually supported resource collection then we
                // need to denote that the links needs to be fixed
                const std::regex collectionRegex(
                    "/redfish/v1/(Chassis|Managers|Systems|Fabrics|ComponentIntegrity)/?");
                if (std::regex_match(targetURI, collectionRegex))
                {
                    BMCWEB_LOG_DEBUG << "Request is for aggregated collection";
                    asyncResp->res.aggregationCollection = true;
                }

                // The request is meant for a valid URI that is not
                // aggregated so we can proceed as normal, no need to
                // remove a URI prefix
                BMCWEB_LOG_DEBUG << "Aggregation not required";
            }
        } // End Redfish Aggregation initial processing

        // Now locally handle the request
        reqHandler(thisReq, asyncResp);
    }

    // Returns all aggregation prefixes including "main" formatted to be used
    // in regex matching
    static void getPrefixes(
        std::string& prefixes,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        prefixes.clear();
        if (satelliteInfo.empty())
        {
            return;
        }

        for (const auto& sat : satelliteInfo)
        {
            prefixes += sat.first + '|';
        }

        prefixes += "main";
        BMCWEB_LOG_DEBUG << "Regex prefix is \"" << prefixes << "\"";
    }

    // Attempt to forward a request to the satellite BMC associated with the
    // prefix.  Set a 404 error in asyncResp if the prefix is not associated
    // with any known satellites
    void forwardRequest(
        crow::Request& thisReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::string& prefix,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        const auto& sat = satelliteInfo.find(prefix);
        if (sat == satelliteInfo.end())
        {
            BMCWEB_LOG_ERROR << "Unrecognized satellite prefix " << prefix;
            asyncResp->res.result(boost::beast::http::status::not_found);
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
            std::bind_front(processResponse, asyncResp);

        std::string data = boost::lexical_cast<std::string>(thisReq.req.body());
        crow::HttpClient::getInstance().sendDataWithCallback(
            data, id, std::string(sat->second.host()),
            sat->second.port_number(), targetURI, thisReq.fields,
            thisReq.method(), retryPolicyName, cb);
    }

    // Processes the response returned by a satellite BMC and loads its
    // contents into asyncResp
    static void
        processResponse(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        crow::Response& resp)
    {
        // No processing needed if the request wasn't successful
        if (resp.resultInt() != 200)
        {
            BMCWEB_LOG_DEBUG << "No need to parse satellite response";
            asyncResp->res.stringResponse = resp.stringResponse;
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
            asyncResp->res.jsonValue = std::move(jsonVal);
        }
        else
        {
            // We received a 200 response without a parsable payload so just
            // copy it as is
            asyncResp->res.stringResponse = resp.stringResponse;
        }
    }

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
    // reqHandler should locally handle thisReq and then write the results
    // into asyncResp
    static void beginAggregation(
        std::function<void(crow::Request&,
                           const std::shared_ptr<bmcweb::AsyncResp>&)>&
            reqHandler,
        crow::Request& thisReq, std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        auto cb = [reqHandler, &thisReq, asyncResp](
                      const std::unordered_map<std::string, boost::urls::url>&
                          satelliteInfo) {
            getInstance().aggregateAndHandle(reqHandler, thisReq, asyncResp,
                                             satelliteInfo);
        };

        getSatelliteConfigs(std::move(cb));
    }
};

// Search the json for all "@odata.id" keys and add the prefix to the URIs
// which are their associated values.
static void addPrefixes(nlohmann::json& json, const std::string& prefix)
{
    for (auto& item : json.items())
    {
        // TODOME: Check to make sure none of these functions throw
        // exceptions

        // URIs we need to fix will appear as the value for "@odata.id" keys
        if (item.key() == "@odata.id")
        {
            std::string tmpVal = json["@odata.id"];
            // Only modify the URI if it's associated with an aggregated
            // resource
            const std::regex urlRegex(
                "/redfish/v1/(Chassis|Managers|Systems|Fabrics|ComponentIntegrity)/.+");
            if (std::regex_match(tmpVal, urlRegex))
            {
                std::vector<std::string> fields;
                boost::split(fields, tmpVal, boost::is_any_of("/"));

                // The resource type will be in fields[3]
                // Add the prefix to the ID and separate it with an
                // underscore
                size_t pos = ("/redfish/v1/" + fields[3] + "/").length();
                tmpVal.insert(pos, prefix + "_");
                json["@odata.id"] = tmpVal;
            }
            continue;
        }

        if (item.value().is_structured())
        {
            addPrefixes(item.value(), prefix);
        }
    }
}

} // namespace redfish
