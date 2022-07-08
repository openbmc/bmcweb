#pragma once
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

NLOHMANN_JSON_SERIALIZE_ENUM(AddressRangeType, {
    {AddressRangeType::Invalid, "Invalid"},
    {AddressRangeType::Volatile, "Volatile"},
    {AddressRangeType::PMEM, "PMEM"},
    {AddressRangeType::Block, "Block"},
});

}
// clang-format on
