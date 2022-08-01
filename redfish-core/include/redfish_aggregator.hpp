#pragma once

#include <http_client.hpp>

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

        // We only need the first 5 URI segments at most to determine if we
        // need to aggregate the request
        std::vector<std::string> segments;
        auto it = thisReq.urlView.segments().begin();
        auto end = thisReq.urlView.segments().end();
        for (size_t index = 0; index < 5; index++)
        {
            if (it == end)
            {
                break;
            }
            segments.push_back(std::move(std::string(*it)));
            it++;
        }

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
            if (segments[2] != "UpdateService")
            {
                BMCWEB_LOG_DEBUG << "Need to forward a collection";
                // TODO: This should instead be handled so that we can
                // aggregate the satellite resource collections

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
            if ((segments[2] == "UpdateService") &&
                ((segments[3] == "FirmwareInventory") ||
                 (segments[3] == "SoftwareInventory")))
            {
                BMCWEB_LOG_DEBUG
                    << "Need to forward a collection under UpdateService";
                // TODO: This should instead be handled so that we can
                // aggregate the satellite resource collections

                // We also need to retrieve the resources from the local BMC
                return Result::LocalHandle;
            }
        }

        // We know that the ID of an aggregated resource will begin with
        // "aggregated".  For the most part the URI will begin like this:
        // /redfish/v1/<resource>/<resource ID>
        if (segments.size() >= 4)
        {
            if (segments[3].starts_with("aggregated"))
            {
                BMCWEB_LOG_DEBUG << "Need to forward a request";

                // TODO: Extract the prefix from the request's URI, retrieve the
                // associated satellite config information, and then forward
                // the request to that satellite.
                redfish::messages::internalError(asyncResp->res);
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

            if (segments[2] != "UpdateService")
            {
                return Result::LocalHandle;
            }
            if ((segments[3] != "FirmwareInventory") &&
                (segments[3] != "SoftwareInventory"))
            {
                return Result::LocalHandle;
            }
            if (segments[4].starts_with("aggregated"))
            {
                BMCWEB_LOG_DEBUG
                    << "Need to forward a request under UpdateService/"
                    << segments[3];

                // TODO: Extract the prefix from the request's URI, retrieve the
                // associated satellite config information, and then forward
                // the request to that satellite.
                redfish::messages::internalError(asyncResp->res);
                return Result::NoLocalHandle;
            }
        }

        BMCWEB_LOG_DEBUG << "Aggregation not required";
        return Result::LocalHandle;
    }
};

} // namespace redfish
