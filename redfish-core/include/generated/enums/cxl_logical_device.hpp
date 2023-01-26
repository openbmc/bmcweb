#pragma once
#include <nlohmann/json.hpp>

namespace cxl_logical_device
{
// clang-format off

enum class CXLSemantic{
    Invalid,
    CXLio,
    CXLcache,
    CXLmem,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CXLSemantic, {
    {CXLSemantic::Invalid, "Invalid"},
    {CXLSemantic::CXLio, "CXLio"},
    {CXLSemantic::CXLcache, "CXLcache"},
    {CXLSemantic::CXLmem, "CXLmem"},
});

}
// clang-format on
