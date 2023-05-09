#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace session
{
// clang-format off

enum class SessionTypes{
    Invalid,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    OEM,
    Redfish,
    VirtualMedia,
    WebUI,
    OutboundConnection,
};

NLOHMANN_JSON_SERIALIZE_ENUM(SessionTypes, {
    {SessionTypes::Invalid, "Invalid"},
    {SessionTypes::HostConsole, "HostConsole"},
    {SessionTypes::ManagerConsole, "ManagerConsole"},
    {SessionTypes::IPMI, "IPMI"},
    {SessionTypes::KVMIP, "KVMIP"},
    {SessionTypes::OEM, "OEM"},
    {SessionTypes::Redfish, "Redfish"},
    {SessionTypes::VirtualMedia, "VirtualMedia"},
    {SessionTypes::WebUI, "WebUI"},
    {SessionTypes::OutboundConnection, "OutboundConnection"},
});

BOOST_DESCRIBE_ENUM(SessionTypes,

    Invalid,
    HostConsole,
    ManagerConsole,
    IPMI,
    KVMIP,
    OEM,
    Redfish,
    VirtualMedia,
    WebUI,
    OutboundConnection,
);

}
// clang-format on
