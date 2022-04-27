#pragma once

#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <http_client.hpp>
#include <http_connection.hpp>

namespace redfish
{

enum class Result
{
    LocalHandle,
    NoLocalHandle
};

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

                    if (!satelliteInfo.empty())
                    {
                        BMCWEB_LOG_ERROR
                            << "Redfish Aggregation only supports one satellite!";
                        BMCWEB_LOG_DEBUG << "Clearing all satellite data";
                        satelliteInfo.clear();
                        return;
                    }

                    // For now assume there will only be one satellite config.
                    // Assign it the name/prefix "aggregated0"
                    addSatelliteConfig("aggregated0", interface.second,
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

    static void
        aggregationHelper(const bool isCollection, const crow::Request& thisReq,
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
            if (!isCollection)
            {
                messages::internalError(asyncResp->res);
            }
            return;
        }

        getSatelliteConfigs(std::bind_front(aggregateAndHandle, isCollection,
                                            localReq, asyncResp));
    }

    // Intended to handle an incoming request based on if Redfish Aggregation
    // is enabled.  Forwards request to satellite BMC if it exists.
    static void aggregateAndHandle(
        const bool isCollection,
        const std::shared_ptr<crow::Request>& sharedReq,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
        const std::unordered_map<std::string, boost::urls::url>& satelliteInfo)
    {
        crow::Request& thisReq = *sharedReq;
        BMCWEB_LOG_DEBUG << "Aggregation is enabled, begin processing of "
                         << thisReq.target();

        // We previously determined the request is for a collection.  No need to
        // check again
        if (isCollection)
        {
            // TODO: This should instead be handled so that we can
            // aggregate the satellite resource collections
            BMCWEB_LOG_DEBUG << "Aggregating a collection";
            return;
        }

        boost::urls::string_view origTarget(thisReq.target().data(),
                                            thisReq.target().size());

        // We need to forward the request if its path begins with form of
        // /redfish/v1/<resource>/<known_prefix>_<resource_id>
        boost::urls::url thisURL(thisReq.urlView);
        const boost::urls::segments& segments = thisURL.segments();

        if (segments.size() >= 4)
        {
            std::string seg2(segments[2]);
            std::string targetURI(segments[3]);

            if ((seg2 == "UpdateService") &&
                ((targetURI == "FirmwareInventory") ||
                 (targetURI == "SoftwareInventory")))
            {
                if (segments.size() > 4)
                {
                    BMCWEB_LOG_DEBUG << "Resource is under UpdateService/"
                                     << targetURI;
                    targetURI = std::move(std::string(segments[4]));
                }
            }

            // Determine if the resource ID begins with a known prefix
            for (const auto& satellite : satelliteInfo)
            {
                const auto& prefix = satellite.first;
                std::string targetPrefix(prefix);
                targetPrefix += "_";
                size_t pos = targetURI.find(targetPrefix);
                if (pos == 0)
                {
                    BMCWEB_LOG_DEBUG << "\"" << prefix
                                     << "\" is a known prefix";

                    // Remove the known prefix from the request's URI and
                    // then forward to the associated satellite BMC
                    getInstance().forwardRequest(thisReq, asyncResp, prefix,
                                                 satelliteInfo);
                    return;
                }
            }
        }

        // This request should have been forwarded, but we didn't recognize the
        // prefix.
        BMCWEB_LOG_DEBUG << "Unrecognized prefix";
        boost::urls::string_value name = thisReq.urlView.segments().back();
        std::string_view nameStr(name.data(), name.size());
        messages::resourceNotFound(asyncResp->res, "", nameStr);
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
            asyncResp->res.stringResponse = std::move(resp.stringResponse);
            return;
        }

        // The resp will not have a json component
        // We need to create a json from resp's stringResponse
        if (resp.getHeaderValue("Content-Type") == "application/json")
        {
            nlohmann::json jsonVal =
                nlohmann::json::parse(resp.body(), nullptr, false);
            if (jsonVal.is_discarded())
            {
                BMCWEB_LOG_ERROR << "Error parsing satellite response as JSON";
                messages::operationFailed(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG << "Successfully parsed satellite response";

            // TODO: For collections we  want to add the satellite responses to
            // our response rather than just straight overwriting them if our
            // local handling was successful (i.e. would return a 200).

            asyncResp->res.stringResponse.emplace(
                boost::beast::http::response<
                    boost::beast::http::string_body>{});
            asyncResp->res.result(resp.result());
            asyncResp->res.jsonValue = std::move(jsonVal);

            BMCWEB_LOG_DEBUG << "Finished writing asyncResp";
            // TODO: Need to fix the URIs in the response so that they include
            // the prefix
        }
        else
        {
            if (!resp.body().empty())
            {
                // We received a 200 response without the correct Content-Type
                // so return an Operation Failed error
                BMCWEB_LOG_ERROR
                    << "Satellite response must be of type \"application/json\"";
                messages::operationFailed(asyncResp->res);
            }
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
    // Returns Result stating whether or not we still need to locally handle the
    // request
    static Result
        beginAggregation(const crow::Request& thisReq,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
    {
        boost::urls::string_view origTarget(thisReq.target().data(),
                                            thisReq.target().size());

        boost::urls::url thisURL(thisReq.urlView);
        boost::urls::segments segments = thisURL.segments();

        // Remove trailing '/' to simplify matching logic
        if ((!segments.empty()) && segments.back().empty())
        {
            segments.pop_back();
        }

        // Is the request for a resource collection?:
        // /redfish/v1/<resource>
        // /redfish/v1/<resource>
        if (segments.size() == 3)
        {
            // /redfish/v1/UpdateService is not a collection
            std::string seg2(segments[2]);
            if (seg2 != "UpdateService")
            {
                BMCWEB_LOG_DEBUG << "Need to forward a collection";
                aggregationHelper(true, thisReq, asyncResp);

                // We also need to retrieve the resources from the local BMC
                return Result::LocalHandle;
            }
        }

        // We also need to aggregate FirmwareInventory and SoftwareInventory
        // as collections
        // /redfish/v1/UpdateService/FirmwareInventory
        // /redfish/v1/UpdateService/SoftwareInventory
        if (segments.size() == 4)
        {
            std::string seg2(segments[2]);
            std::string seg3(segments[3]);
            if ((seg2 == "UpdateService") && ((seg3 == "FirmwareInventory") ||
                                              (seg3 == "SoftwareInventory")))
            {
                BMCWEB_LOG_DEBUG
                    << "Need to forward a collection under UpdateService";
                aggregationHelper(true, thisReq, asyncResp);

                // We also need to retrieve the resources from the local BMC
                return Result::LocalHandle;
            }
        }

        // We know that the ID of an aggregated resource will begin with
        // "aggregated".  For the most part the URI will begin like this:
        // /redfish/v1/<resource>/<resource ID>
        if (segments.size() >= 4)
        {
            std::string targetURI(segments[3]);
            std::string targetPrefix("aggregated");
            size_t pos = targetURI.find(targetPrefix);
            if (pos == 0)
            {
                BMCWEB_LOG_DEBUG << "Need to forward a request";

                // Extract the prefix from the request's URI, retrieve the
                // associated satellite config information, and then forward
                // the request to that satellite.
                aggregationHelper(false, thisReq, asyncResp);
                return Result::NoLocalHandle;
            }
        }

        // Make sure this isn't for one of the aggregated resources located
        // under UpdateService:
        // /redfish/v1/UpdateService/FirmwareInventory/<FirmwareInventory ID>
        // /redfish/v1/UpdateService/SoftwareInventory/<SoftwareInventory ID>
        if (segments.size() >= 5)
        {
            std::string seg2(segments[2]);
            std::string seg3(segments[3]);
            std::string seg4(segments[4]);

            if (seg2 != "UpdateService")
            {
                return Result::LocalHandle;
            }
            if ((seg3 != "FirmwareInventory") && (seg3 != "SoftwareInventory"))
            {
                return Result::LocalHandle;
            }
            size_t pos = seg4.find("aggregated");
            if (pos == 0)
            {
                BMCWEB_LOG_DEBUG
                    << "Need to forward a request under UpdateService/" << seg3;

                // Extract the prefix from the request's URI, retrieve the
                // associated satellite config information, and then forward
                // the request to that satellite.
                aggregationHelper(false, thisReq, asyncResp);
                return Result::NoLocalHandle;
            }
        }

        BMCWEB_LOG_DEBUG << "Aggregation not required";
        return Result::LocalHandle;
    }
};

} // namespace redfish
