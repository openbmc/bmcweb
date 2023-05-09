#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace collection_capabilities
{
// clang-format off

enum class UseCase{
    Invalid,
    ComputerSystemComposition,
    ComputerSystemConstrainedComposition,
    VolumeCreation,
    ResourceBlockComposition,
    ResourceBlockConstrainedComposition,
    RegisterResourceBlock,
};

NLOHMANN_JSON_SERIALIZE_ENUM(UseCase, {
    {UseCase::Invalid, "Invalid"},
    {UseCase::ComputerSystemComposition, "ComputerSystemComposition"},
    {UseCase::ComputerSystemConstrainedComposition, "ComputerSystemConstrainedComposition"},
    {UseCase::VolumeCreation, "VolumeCreation"},
    {UseCase::ResourceBlockComposition, "ResourceBlockComposition"},
    {UseCase::ResourceBlockConstrainedComposition, "ResourceBlockConstrainedComposition"},
    {UseCase::RegisterResourceBlock, "RegisterResourceBlock"},
});

BOOST_DESCRIBE_ENUM(UseCase,

    Invalid,
    ComputerSystemComposition,
    ComputerSystemConstrainedComposition,
    VolumeCreation,
    ResourceBlockComposition,
    ResourceBlockConstrainedComposition,
    RegisterResourceBlock,
);

}
// clang-format on
