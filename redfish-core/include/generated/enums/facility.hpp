#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace facility
{
// clang-format off

enum class FacilityType{
    Invalid,
    Room,
    Floor,
    Building,
    Site,
};

NLOHMANN_JSON_SERIALIZE_ENUM(FacilityType, {
    {FacilityType::Invalid, "Invalid"},
    {FacilityType::Room, "Room"},
    {FacilityType::Floor, "Floor"},
    {FacilityType::Building, "Building"},
    {FacilityType::Site, "Site"},
});

BOOST_DESCRIBE_ENUM(FacilityType,

    Invalid,
    Room,
    Floor,
    Building,
    Site,
);

}
// clang-format on
