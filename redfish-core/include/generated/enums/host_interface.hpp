#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace host_interface
{
// clang-format off

enum class HostInterfaceType{
    Invalid,
    NetworkHostInterface,
};

enum class AuthenticationMode{
    Invalid,
    AuthNone,
    BasicAuth,
    RedfishSessionAuth,
    OemAuth,
};

NLOHMANN_JSON_SERIALIZE_ENUM(HostInterfaceType, {
    {HostInterfaceType::Invalid, "Invalid"},
    {HostInterfaceType::NetworkHostInterface, "NetworkHostInterface"},
});

BOOST_DESCRIBE_ENUM(HostInterfaceType,

    Invalid,
    NetworkHostInterface,
);

NLOHMANN_JSON_SERIALIZE_ENUM(AuthenticationMode, {
    {AuthenticationMode::Invalid, "Invalid"},
    {AuthenticationMode::AuthNone, "AuthNone"},
    {AuthenticationMode::BasicAuth, "BasicAuth"},
    {AuthenticationMode::RedfishSessionAuth, "RedfishSessionAuth"},
    {AuthenticationMode::OemAuth, "OemAuth"},
});

BOOST_DESCRIBE_ENUM(AuthenticationMode,

    Invalid,
    AuthNone,
    BasicAuth,
    RedfishSessionAuth,
    OemAuth,
);

}
// clang-format on
