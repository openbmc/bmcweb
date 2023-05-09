#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace media_controller
{
// clang-format off

enum class MediaControllerType{
    Invalid,
    Memory,
};

NLOHMANN_JSON_SERIALIZE_ENUM(MediaControllerType, {
    {MediaControllerType::Invalid, "Invalid"},
    {MediaControllerType::Memory, "Memory"},
});

BOOST_DESCRIBE_ENUM(MediaControllerType,

    Invalid,
    Memory,
);

}
// clang-format on
