#pragma once
#include <nlohmann/json.hpp>

namespace storage_controller
{
// clang-format off

enum class NVMeControllerType{
    Invalid,
    Admin,
    Discovery,
    IO,
};

enum class ANAAccessState{
    Invalid,
    Optimized,
    NonOptimized,
    Inaccessible,
    PersistentLoss,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NVMeControllerType, {
    {NVMeControllerType::Invalid, "Invalid"},
    {NVMeControllerType::Admin, "Admin"},
    {NVMeControllerType::Discovery, "Discovery"},
    {NVMeControllerType::IO, "IO"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ANAAccessState, {
    {ANAAccessState::Invalid, "Invalid"},
    {ANAAccessState::Optimized, "Optimized"},
    {ANAAccessState::NonOptimized, "NonOptimized"},
    {ANAAccessState::Inaccessible, "Inaccessible"},
    {ANAAccessState::PersistentLoss, "PersistentLoss"},
});

}
// clang-format on
