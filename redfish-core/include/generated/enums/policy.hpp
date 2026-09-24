// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace policy
{
// clang-format off

enum class ConditionType{
    Invalid,
    And,
    Or,
    Compare,
    Read,
    Sum,
    Average,
    Event,
};

enum class ResponseAction{
    Invalid,
    Recheck,
    Event,
    Reset,
    HTTP,
    OEM,
};

enum class Operator{
    Invalid,
    Equal,
    NotEqual,
    GreaterThan,
    LessThan,
    GreaterThanOrEqual,
    LessThanOrEqual,
};

enum class HTTPVerb{
    Invalid,
    GET,
    PATCH,
    POST,
    DELETE,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ConditionType, {
    {ConditionType::Invalid, "Invalid"},
    {ConditionType::And, "And"},
    {ConditionType::Or, "Or"},
    {ConditionType::Compare, "Compare"},
    {ConditionType::Read, "Read"},
    {ConditionType::Sum, "Sum"},
    {ConditionType::Average, "Average"},
    {ConditionType::Event, "Event"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(ResponseAction, {
    {ResponseAction::Invalid, "Invalid"},
    {ResponseAction::Recheck, "Recheck"},
    {ResponseAction::Event, "Event"},
    {ResponseAction::Reset, "Reset"},
    {ResponseAction::HTTP, "HTTP"},
    {ResponseAction::OEM, "OEM"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(Operator, {
    {Operator::Invalid, "Invalid"},
    {Operator::Equal, "Equal"},
    {Operator::NotEqual, "NotEqual"},
    {Operator::GreaterThan, "GreaterThan"},
    {Operator::LessThan, "LessThan"},
    {Operator::GreaterThanOrEqual, "GreaterThanOrEqual"},
    {Operator::LessThanOrEqual, "LessThanOrEqual"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(HTTPVerb, {
    {HTTPVerb::Invalid, "Invalid"},
    {HTTPVerb::GET, "GET"},
    {HTTPVerb::PATCH, "PATCH"},
    {HTTPVerb::POST, "POST"},
    {HTTPVerb::DELETE, "DELETE"},
});

// clang-format on
} // namespace policy
