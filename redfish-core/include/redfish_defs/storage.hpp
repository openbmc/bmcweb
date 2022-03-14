#pragma once
#include <nlohmann/json.hpp>

namespace storage
{
// clang-format off

enum class ResetToDefaultsType{
    Invalid,
    ResetAll,
    PreserveVolumes,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ResetToDefaultsType, { //NOLINT
    {ResetToDefaultsType::Invalid, "Invalid"},
    {ResetToDefaultsType::ResetAll, "ResetAll"},
    {ResetToDefaultsType::PreserveVolumes, "PreserveVolumes"},
});

}
// clang-format on
