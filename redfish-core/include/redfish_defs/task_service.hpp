#pragma once
#include <nlohmann/json.hpp>

namespace task_service
{
// clang-format off

enum class OverWritePolicy{
    Invalid,
    Manual,
    Oldest,
};

NLOHMANN_JSON_SERIALIZE_ENUM(OverWritePolicy, { //NOLINT
    {OverWritePolicy::Invalid, "Invalid"},
    {OverWritePolicy::Manual, "Manual"},
    {OverWritePolicy::Oldest, "Oldest"},
});

}
// clang-format on
