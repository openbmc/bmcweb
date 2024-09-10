// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

#include <iostream>
#include <list>
#include <numeric>
#include <optional>
#include <string>

namespace redfish
{
namespace filter_ast
{

namespace x3 = boost::spirit::x3;

// Represents a string literal
// (ie 'Enabled')
struct QuotedString : std::string
{};

// Represents a string that matches an identifier
// Looks similar to a json pointer
struct UnquotedString : std::string
{};

// Because some program elements can reference BooleanOp recursively,
// they need to be forward declared so that x3::forward_ast can be used
struct LogicalAnd;
struct Comparison;
using BooleanOp =
    x3::variant<x3::forward_ast<Comparison>, x3::forward_ast<LogicalAnd>>;

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

using Argument = x3::variant<int64_t, double, UnquotedString, QuotedString>;

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

BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::Comparison, left, token, right);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalNot, isLogicalNot,
                          operand);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalOr, first, rest);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalAnd, first, rest);
