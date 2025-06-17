// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace job_document
{
// clang-format off

enum class DataType{
    Invalid,
    Boolean,
    Number,
    String,
};

NLOHMANN_JSON_SERIALIZE_ENUM(DataType, {
    {DataType::Invalid, "Invalid"},
    {DataType::Boolean, "Boolean"},
    {DataType::Number, "Number"},
    {DataType::String, "String"},
});

}
// clang-format on
