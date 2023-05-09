#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace secure_boot_database
{
// clang-format off

enum class ResetKeysType{
    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResetKeysType, {
    {ResetKeysType::Invalid, "Invalid"},
    {ResetKeysType::ResetAllKeysToDefault, "ResetAllKeysToDefault"},
    {ResetKeysType::DeleteAllKeys, "DeleteAllKeys"},
});

BOOST_DESCRIBE_ENUM(ResetKeysType,

    Invalid,
    ResetAllKeysToDefault,
    DeleteAllKeys,
);

}
// clang-format on
