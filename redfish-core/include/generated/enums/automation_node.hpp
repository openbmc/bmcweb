// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <nlohmann/json.hpp>

namespace automation_node
{
// clang-format off

enum class NodeState{
    Invalid,
    Idle,
    Done,
    Waiting,
    ConditionStop,
    ErrorStop,
    Running,
};

enum class NodeType{
    Invalid,
    MotionPosition,
    MotionVelocity,
    MotionPositionGroup,
    PID,
    Simple,
};

enum class MotionProfileType{
    Invalid,
    Trapezoidal,
    SCurve,
    None,
};

enum class MotionAxisType{
    Invalid,
    X,
    Y,
    Z,
    TwoAxis,
    ThreeAxis,
};

NLOHMANN_JSON_SERIALIZE_ENUM(NodeState, {
    {NodeState::Invalid, "Invalid"},
    {NodeState::Idle, "Idle"},
    {NodeState::Done, "Done"},
    {NodeState::Waiting, "Waiting"},
    {NodeState::ConditionStop, "ConditionStop"},
    {NodeState::ErrorStop, "ErrorStop"},
    {NodeState::Running, "Running"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(NodeType, {
    {NodeType::Invalid, "Invalid"},
    {NodeType::MotionPosition, "MotionPosition"},
    {NodeType::MotionVelocity, "MotionVelocity"},
    {NodeType::MotionPositionGroup, "MotionPositionGroup"},
    {NodeType::PID, "PID"},
    {NodeType::Simple, "Simple"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MotionProfileType, {
    {MotionProfileType::Invalid, "Invalid"},
    {MotionProfileType::Trapezoidal, "Trapezoidal"},
    {MotionProfileType::SCurve, "SCurve"},
    {MotionProfileType::None, "None"},
});

NLOHMANN_JSON_SERIALIZE_ENUM(MotionAxisType, {
    {MotionAxisType::Invalid, "Invalid"},
    {MotionAxisType::X, "X"},
    {MotionAxisType::Y, "Y"},
    {MotionAxisType::Z, "Z"},
    {MotionAxisType::TwoAxis, "TwoAxis"},
    {MotionAxisType::ThreeAxis, "ThreeAxis"},
});

}
// clang-format on
