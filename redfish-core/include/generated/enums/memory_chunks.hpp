#pragma once
#include <boost/describe/enum.hpp>
#include <nlohmann/json.hpp>

namespace memory_chunks
{
// clang-format off

enum class AddressRangeType{
    Invalid,
    Volatile,
    PMEM,
    Block,
};

enum class MediaLocation{
    Invalid,
    Local,
    Remote,
    Mixed,
};

enum class OperationalState{
    Invalid,
    Online,
    Offline,
};

NLOHMANN_JSON_SERIALIZE_ENUM(AddressRangeType, {
    {AddressRangeType::Invalid, "Invalid"},
    {AddressRangeType::Volatile, "Volatile"},
    {AddressRangeType::PMEM, "PMEM"},
    {AddressRangeType::Block, "Block"},
});

BOOST_DESCRIBE_ENUM(AddressRangeType,

    Invalid,
    Volatile,
    PMEM,
    Block,
);

NLOHMANN_JSON_SERIALIZE_ENUM(MediaLocation, {
    {MediaLocation::Invalid, "Invalid"},
    {MediaLocation::Local, "Local"},
    {MediaLocation::Remote, "Remote"},
    {MediaLocation::Mixed, "Mixed"},
});

BOOST_DESCRIBE_ENUM(MediaLocation,

    Invalid,
    Local,
    Remote,
    Mixed,
);

NLOHMANN_JSON_SERIALIZE_ENUM(OperationalState, {
    {OperationalState::Invalid, "Invalid"},
    {OperationalState::Online, "Online"},
    {OperationalState::Offline, "Offline"},
});

BOOST_DESCRIBE_ENUM(OperationalState,

    Invalid,
    Online,
    Offline,
);

}
// clang-format on
