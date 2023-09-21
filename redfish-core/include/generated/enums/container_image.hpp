#pragma once
#include <nlohmann/json.hpp>

namespace container_image
{
// clang-format off

enum class ImageTypes{
    Invalid,
    DockerV1,
    DockerV2,
    OCI,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ImageTypes, {
    {ImageTypes::Invalid, "Invalid"},
    {ImageTypes::DockerV1, "DockerV1"},
    {ImageTypes::DockerV2, "DockerV2"},
    {ImageTypes::OCI, "OCI"},
});

}
// clang-format on
