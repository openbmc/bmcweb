#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace registered_client
{
// clang-format off

enum class ClientType{
    Invalid,
    Monitor,
    Configure,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ClientType, {
    {ClientType::Invalid, "Invalid"},
    {ClientType::Monitor, "Monitor"},
    {ClientType::Configure, "Configure"},
});

BOOST_DESCRIBE_ENUM(ClientType,

    Invalid,
    Monitor,
    Configure,
);

}
// clang-format on
