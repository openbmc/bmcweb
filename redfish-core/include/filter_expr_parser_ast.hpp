#pragma once
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

#include <iostream>
#include <list>
#include <numeric>
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

// Because some program elements can reference BooleanOperation recursively,
// they need to be forward declared so that x3::forward_ast can be used
struct LogicalAnd;
struct RelationalComparison;
using BooleanOperation = x3::variant<x3::forward_ast<RelationalComparison>,
                                     x3::forward_ast<LogicalAnd>>;

enum class ComparisonOperator
{
    Invalid,
    Equals,
    NotEquals,
    GreaterThan,
    GreaterThanOrEqual,
    LessThan,
    LessThanOrEqual
};

inline x3::symbols<ComparisonOperator> comparisonOperator()
{
    x3::symbols<ComparisonOperator> ret;
    ret.add("gt", ComparisonOperator::GreaterThan);
    ret.add("ge", ComparisonOperator::GreaterThanOrEqual);
    ret.add("lt", ComparisonOperator::LessThan);
    ret.add("le", ComparisonOperator::LessThanOrEqual);
    ret.add("ne", ComparisonOperator::NotEquals);
    ret.add("eq", ComparisonOperator::Equals);
    return ret;
}

// An expression that has been negated with not
struct LogicalNot
{
    std::optional<char> isLogicalNot;
    BooleanOperation operand;
};

using Argument = x3::variant<int64_t, double, UnquotedString, QuotedString>;

struct RelationalComparison
{
    Argument left;
    ComparisonOperator token = ComparisonOperator::Invalid;
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

BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::RelationalComparison, left,
                          token, right);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalNot, isLogicalNot,
                          operand);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalOr, first, rest);
BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::LogicalAnd, first, rest);
