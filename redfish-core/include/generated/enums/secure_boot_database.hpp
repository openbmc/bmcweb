#pragma once
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

}
// clang-format on
