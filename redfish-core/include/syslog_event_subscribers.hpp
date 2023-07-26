#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>

#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/event_destination.hpp"
#include "logging.hpp"
#include "utils/dbus_utils.hpp"

// Note: The following logic is implemented in accordance with the fact that
// phosphor-rsyslog-conf supports only one rsyslog server.

namespace redfish
{

namespace syslog
{

static constexpr const char* syslogConfSvc =
    "xyz.openbmc_project.Syslog.Config";
static constexpr const char* syslogConfPath =
    "/xyz/openbmc_project/logging/config/remote";
static constexpr const char* networkIface =
    "xyz.openbmc_project.Network.Client";
static constexpr const char* syslogFilterIface =
    "xyz.openbmc_project.Logging.Syslog.Config.Filter";
static constexpr const char* propAddress = "Address";
static constexpr const char* propPort = "Port";
static constexpr const char* propSelectors = "Selectors";

#define PREFIX_FACILITY \
    "xyz.openbmc_project.Logging.Syslog.Config.Filter.Facility."
#define PREFIX_PRIO \
    "xyz.openbmc_project.Logging.Syslog.Config.Filter.Priority."
#define PREFIX_PRIO_MOD \
    "xyz.openbmc_project.Logging.Syslog.Config.Filter.PriorityModifier."

#define FACILITY_ALL      PREFIX_FACILITY"All"
#define PRIO_ALL          PREFIX_PRIO"All"
#define PRIO_MOD_NO_MOD   PREFIX_PRIO_MOD"NoModifier"

#define DBUS_TO_REDFISH_FN(name, enumName, prefix)                             \
    inline std::optional<std::string>                                          \
        dbusToRedfish##name(const std::string& s)                              \
    {                                                                          \
        using ns = ::event_destination::enumName;                              \
                                                                               \
        if (!s.starts_with(prefix))                                            \
        {                                                                      \
            return {};                                                         \
        }                                                                      \
                                                                               \
        auto subs = s.substr(sizeof(prefix) - 1);                              \
        auto enumval = nlohmann::json(subs).get<ns>();                         \
                                                                               \
        if (enumval == ns::Invalid)                                            \
        {                                                                      \
            return {};                                                         \
        }                                                                      \
                                                                               \
        return subs;                                                           \
    }

#define DBUS_FROM_REDFISH_FN(name, enumName, prefix)                           \
    inline std::optional<std::string>                                          \
        dbusFromRedfish##name(const std::string& s)                            \
    {                                                                          \
        using ns = ::event_destination::enumName;                              \
        auto enumval = nlohmann::json(s).get<ns>();                            \
                                                                               \
        if (enumval == ns::Invalid)                                            \
        {                                                                      \
            return {};                                                         \
        }                                                                      \
                                                                               \
        return prefix + s;                                                     \
    }

using selectorType =
    std::tuple<std::vector<std::string>, std::string, std::string>;

static const std::vector<std::string> FACILITY_ALL_VEC = { FACILITY_ALL };
static const selectorType FILTER_ALL =
    std::make_tuple(FACILITY_ALL_VEC, PRIO_MOD_NO_MOD, PRIO_ALL);

namespace internal
{

template <typename Type>
boost::system::error_code getDbusProperty(
    boost::asio::yield_context yield,
    const std::string& service, const std::string& objPath,
    const std::string& interface, const std::string& property,
    Type& value)
{
    boost::system::error_code ec;
    sdbusplus::asio::connection* bus = crow::connections::systemBus;

    auto variant = bus->yield_method_call<std::variant<Type>>(
        yield, ec, service, objPath,
        "org.freedesktop.DBus.Properties", "Get",
        interface, property);

    if (!ec)
    {
        Type* pval = std::get_if<Type>(&variant);
        if (pval)
        {
            value = *pval;
            return ec;
        }
        ec = boost::system::errc::make_error_code(
            boost::system::errc::invalid_argument);
    }

    return ec;
}

} // namespace internal

DBUS_FROM_REDFISH_FN(Facility, SyslogFacility, PREFIX_FACILITY)
DBUS_FROM_REDFISH_FN(Priority, SyslogSeverity, PREFIX_PRIO)
DBUS_TO_REDFISH_FN(Facility, SyslogFacility, PREFIX_FACILITY)
DBUS_TO_REDFISH_FN(Priority, SyslogSeverity, PREFIX_PRIO)

inline std::optional
<std::tuple<std::string, uint16_t, std::vector<selectorType>>> getSyslogRule(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    std::string host;
    uint16_t port;
    std::vector<selectorType> selectors;

    ec = internal::getDbusProperty(yield, syslogConfSvc, syslogConfPath,
        syslogFilterIface, propSelectors, selectors);

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propSelectors, ec);
        messages::internalError(asyncResp->res);
        return {};
    }

    ec = internal::getDbusProperty(yield, syslogConfSvc, syslogConfPath,
        networkIface, propAddress, host);

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propAddress, ec);
        messages::internalError(asyncResp->res);
        return {};
    }

    ec = internal::getDbusProperty(yield, syslogConfSvc, syslogConfPath,
        networkIface, propPort, port);

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propPort, ec);
        messages::internalError(asyncResp->res);
        return {};
    }

    return std::make_tuple(host, port, selectors);
}

inline bool syslogSubscriptionExist(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::string& host,
    uint16_t& port,
    std::vector<selectorType>& selectors,
    boost::asio::yield_context yield)
{
    auto rule = getSyslogRule(asyncResp, yield);
    if (!rule.has_value())
    {
        // DBus error
        return false;
    }

    std::tie(host, port, selectors) = *rule;
    return !(host.empty() || !port);
}

inline void collectSyslogSubscriptions(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    boost::asio::spawn(
        crow::connections::systemBus->get_io_context(),
        [asyncResp](boost::asio::yield_context yield) {
            std::string host;
            uint16_t port;
            std::vector<selectorType> selectors;

            if (!syslogSubscriptionExist(
                    asyncResp, host, port, selectors, yield))
            {
                return;
            }

            nlohmann::json::object_t member;
            member["@odata.id"] = boost::urls::format(
                "/redfish/v1/EventService/Subscriptions/{}", "syslog");

            nlohmann::json& memberArr = asyncResp->res.jsonValue["Members"];
            memberArr.push_back(std::move(member));
            asyncResp->res.jsonValue["Members@odata.count"] = memberArr.size();
            return;
        }
    );
}

inline bool validateProtocol(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& protocol)
{
    using ns = ::event_destination::EventDestinationProtocol;
    auto protocolEnum = nlohmann::json(protocol).get<ns>();

    if (protocolEnum == ns::Invalid)
    {
        messages::propertyValueNotInList(asyncResp->res, protocol, "Protocol");
        return false;
    }

    if (protocolEnum != ns::SyslogTLS &&
        protocolEnum != ns::SyslogTCP &&
        protocolEnum != ns::SyslogUDP &&
        protocolEnum != ns::SyslogRELP)
    {
        messages::propertyValueConflict(asyncResp->res,
                                        "Protocol", "SubscriptionType");
        return false;
    }

    if (protocolEnum == ns::SyslogTLS ||
        protocolEnum == ns::SyslogUDP ||
        protocolEnum == ns::SyslogRELP)
    {
        // TCP support only
        messages::propertyValueIncorrect(asyncResp->res, "Protocol", protocol);
        return false;
    }

    return true;
}

inline bool processFacilities(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<std::string>& facilities,
    std::vector<std::string>& dbus)
{
    // according to spec
    if (facilities.empty())
    {
        dbus.push_back(FACILITY_ALL);
        return true;
    }

    for (auto& facility : facilities)
    {
        auto opt = dbusFromRedfishFacility(facility);
        if (!opt.has_value())
        {
            messages::propertyValueNotInList(
                asyncResp->res, facility, "LogFacilities");
            return false;
        }

        dbus.push_back(*opt);
    }

    return true;
}

inline bool processSeverity(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& severity,
    std::string& dbus)
{
    auto opt = dbusFromRedfishPriority(severity);
    if (!opt.has_value())
    {
        messages::propertyValueNotInList(
            asyncResp->res, severity, "LowestSeverity");
        return false;
    }

    dbus = *opt;
    return true;
}

inline bool dbusFromRedfishFilters(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    std::vector<nlohmann::json>& filters,
    std::vector<selectorType>& dbus)
{
    // according to spec
    if (filters.empty())
    {
        dbus.push_back(FILTER_ALL);
        return true;
    }

    for (nlohmann::json& filter : filters)
    {
        std::string severity;
        std::vector<std::string> facilities;
        std::string dbusSeverity;
        std::vector<std::string> dbusFacilities;

        if (!json_util::readJson(
                filter, asyncResp->res,
                "LogFacilities", facilities,
                "LowestSeverity", severity))
        {
            return false;
        }

        if (!processFacilities(asyncResp, facilities, dbusFacilities) ||
            !processSeverity(asyncResp, severity, dbusSeverity))
        {
            return false;
        }

        // Redfish limitation: syslog priority modifiers are not supported
        dbus.emplace_back(dbusFacilities, PRIO_MOD_NO_MOD,dbusSeverity);
    }

    return true;
}

inline bool setSyslogRule(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& host,
    const uint16_t& port,
    const std::vector<selectorType>& selectors,
    boost::asio::yield_context yield)
{
    boost::system::error_code ec;
    sdbusplus::asio::connection* bus = crow::connections::systemBus;

    bus->yield_method_call<>(yield, ec, syslogConfSvc, syslogConfPath,
        "org.freedesktop.DBus.Properties", "Set",
        syslogFilterIface, propSelectors,
        std::variant<std::vector<selectorType>>(selectors));

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propSelectors, ec);
        messages::internalError(asyncResp->res);
        return false;
    }

    bus->yield_method_call<>(yield, ec, syslogConfSvc, syslogConfPath,
        "org.freedesktop.DBus.Properties", "Set",
        networkIface, propAddress, std::variant<std::string>(host));

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propAddress, ec);
        messages::internalError(asyncResp->res);
        return false;
    }

    bus->yield_method_call<>(yield, ec, syslogConfSvc, syslogConfPath,
        "org.freedesktop.DBus.Properties", "Set",
        networkIface, propPort, std::variant<uint16_t>(port));

    if (ec)
    {
        BMCWEB_LOG_ERROR("DBUS error: property ({}),  error ({})",
                         propPort, ec);
        messages::internalError(asyncResp->res);
        return false;
    }

    return true;
}

inline void addSyslogSubscription(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& urlProto,
    const std::string& host,
    const uint16_t& port,
    const std::string& protocol,
    std::optional<std::vector<nlohmann::json>>& filters)
{
    // filters are required
    if (!filters)
    {
        messages::propertyMissing(asyncResp->res, "SyslogFilters");
        return;
    }

    if (urlProto != "syslog")
    {
        messages::propertyValueConflict(
            asyncResp->res, "Destination", "SubscriptionType");
        return;
    }

    std::vector<selectorType> filtersDbus;

    if (!validateProtocol(asyncResp, protocol) ||
        !dbusFromRedfishFilters(asyncResp, *filters, filtersDbus))
    {
        return;
    }

    boost::asio::spawn(
        crow::connections::systemBus->get_io_context(),
        [asyncResp, host, port, filtersDbus](boost::asio::yield_context yield) {
            if (!setSyslogRule(asyncResp, host, port, filtersDbus, yield))
            {
                return;
            }

            boost::urls::url uri = boost::urls::format(
                    "/redfish/v1/EventService/Subscriptions/{}", "syslog");
            asyncResp->res.addHeader("Location", uri.buffer());
            messages::created(asyncResp->res);
        }
    );
}

inline bool fillRespFacilities(
    const std::vector<std::string>& facilities,
    nlohmann::json::object_t& json)
{
    for (const auto& facility : facilities)
    {
        if (facility == FACILITY_ALL)
        {
            // according to spec: all facilities == empty array
            static const std::vector<std::string> v;
            json["LogFacilities"] = v;
            return true;
        }

        auto opt = dbusToRedfishFacility(facility);
        if (!opt.has_value())
        {
            BMCWEB_LOG_ERROR("Invalid facility DBus value: {}", facility);
            return false;
        }
        json["LogFacilities"].push_back(*opt);
    }

    return true;
}

inline bool fillRespPriority(
    const std::string& priority,
    nlohmann::json::object_t& json)
{
    auto opt = dbusToRedfishPriority(priority);
    if (!opt.has_value())
    {
        BMCWEB_LOG_ERROR("Invalid priority DBus value: {}", priority);
        return false;
    }
    json["LowestSeverity"] = *opt;
    return true;
}

inline void fillRespSyslogFilters(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::vector<selectorType> selectors)
{
    nlohmann::json& filterArray = asyncResp->res.jsonValue["SyslogFilters"];

    for (const auto& s : selectors)
    {
        nlohmann::json::object_t json;
        const auto& [facilities, mod, prio] = s;

        // facilities, priority
        if (!fillRespFacilities(facilities, json) ||
            !fillRespPriority(prio, json))
        {
            messages::internalError(asyncResp->res);
            return;
        }

        // modifier
        if (mod != PRIO_MOD_NO_MOD)
        {
            // Redfish limitation: syslog priority modifiers are not supported
            BMCWEB_LOG_ERROR("Redfish limitation: syslog priority modifiers"
                " are not supported. Parsed non-empty syslog priority."
                " Please check rsyslog config.");
            messages::internalError(asyncResp->res);
            return;
        }

        filterArray.push_back(json);
    }
}

inline void getSyslogSubscription(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& id)
{
    if (id != "syslog")
    {
        // phosphor-rsyslog-conf supports only one rsyslog server.
        messages::resourceNotFound(asyncResp->res, "Subscription", id);
        return;
    }

    boost::asio::spawn(
        crow::connections::systemBus->get_io_context(),
        [asyncResp, id](boost::asio::yield_context yield) {
            std::string host;
            uint16_t port;
            std::vector<selectorType> selectors;

            if (!syslogSubscriptionExist(
                    asyncResp, host, port, selectors, yield))
            {
                messages::resourceNotFound(asyncResp->res, "Subscription", id);
                return;
            }

            asyncResp->res.jsonValue["@odata.type"] =
                "#EventDestination.v1_9_0.EventDestination";
            asyncResp->res.jsonValue["Protocol"] = "SyslogTCP";
            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/EventService/Subscriptions/{}", id);
            asyncResp->res.jsonValue["Id"] = id;
            asyncResp->res.jsonValue["Name"] = "Event Destination";
            asyncResp->res.jsonValue["SubscriptionType"] = "Syslog";
            asyncResp->res.jsonValue["EventFormatType"] = "Event";
            asyncResp->res.jsonValue["Context"] = "";
            asyncResp->res.jsonValue["Destination"] =
                boost::urls::format("syslog://{}:{}", host, port);
            fillRespSyslogFilters(asyncResp, selectors);
        }
    );
}

inline void deleteSyslogSubscription(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& id)
{
    if (id != "syslog")
    {
        // phosphor-rsyslog-conf supports only one rsyslog server.
        messages::resourceNotFound(asyncResp->res, "Subscription", id);
        return;
    }

    boost::asio::spawn(
        crow::connections::systemBus->get_io_context(),
        [asyncResp, id](boost::asio::yield_context yield) {
            std::string host;
            uint16_t port;
            std::vector<selectorType> selectors;

            if (!syslogSubscriptionExist(
                    asyncResp, host, port, selectors, yield))
            {
                messages::resourceNotFound(asyncResp->res, "Subscription", id);
                return;
            }

            if (!setSyslogRule(asyncResp, "", 0, {FILTER_ALL}, yield))
            {
                return;
            }
            messages::success(asyncResp->res);
        }
    );
}

} // namespace syslog
} // namespace redfish
