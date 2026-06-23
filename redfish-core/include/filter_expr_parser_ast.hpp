// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <boost/variant/recursive_wrapper.hpp>

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <variant>

namespace redfish
{
namespace filter_ast
{

// Represents a string literal
// (ie 'Enabled')
struct QuotedString : std::string
{};

// Represents a string that matches an identifier
// Looks similar to a json pointer
struct UnquotedString : std::string
{};

// Forward declared so recursive_wrapper can break the recursive type
struct LogicalAnd;
struct Comparison;
using BooleanOp = std::variant<boost::recursive_wrapper<Comparison>,
                               boost::recursive_wrapper<LogicalAnd>>;

enum class ComparisonOpEnum
{
    Invalid,
    Equals,
    NotEquals,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual
};

// An expression that has been negated with not
struct LogicalNot
{
    std::optional<char> isLogicalNot;
    BooleanOp operand;
};

using Argument = std::variant<int64_t, double, UnquotedString, QuotedString>;

struct Comparison
{
    Argument left;
    ComparisonOpEnum token = ComparisonOpEnum::Invalid;
    Argument right;
};

struct LogicalOr
{
    LogicalNot first;
    std::list<LogicalNot> rest;
};
struct LogicalAnd
{
    LogicalOr first;
    std::list<LogicalOr> rest;
};
} // namespace filter_ast
} // namespace redfish
