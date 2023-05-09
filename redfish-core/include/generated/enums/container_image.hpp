#pragma once
#include <boost/describe/enum.hpp>
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

BOOST_DESCRIBE_ENUM(ImageTypes,

    Invalid,
    DockerV1,
    DockerV2,
    OCI,
);

}
// clang-format on
