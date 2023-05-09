#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace thermal
{
// clang-format off

enum class ReadingUnits{
    Invalid,
    RPM,
    Percent,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ReadingUnits, {
    {ReadingUnits::Invalid, "Invalid"},
    {ReadingUnits::RPM, "RPM"},
    {ReadingUnits::Percent, "Percent"},
});

BOOST_DESCRIBE_ENUM(ReadingUnits,

    Invalid,
    RPM,
    Percent,
);

}
// clang-format on
