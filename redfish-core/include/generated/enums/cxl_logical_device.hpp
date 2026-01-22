// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
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

enum class PassphraseType{
    Invalid,
    User,
    Master,
};

NLOHMANN_JSON_SERIALIZE_ENUM(CXLSemantic, {
    {CXLSemantic::Invalid, "Invalid"},
    {CXLSemantic::CXLio, "CXLio"},
    {CXLSemantic::CXLcache, "CXLcache"},
    {CXLSemantic::CXLmem, "CXLmem"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(PassphraseType, {
    {PassphraseType::Invalid, "Invalid"},
    {PassphraseType::User, "User"},
    {PassphraseType::Master, "Master"},
});

// clang-format on
}
