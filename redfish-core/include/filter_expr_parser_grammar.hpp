// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "filter_expr_parser_ast.hpp"

#include <boost/spirit/home/x3/char/char.hpp>
#include <boost/spirit/home/x3/char/negated_char_parser.hpp>
#include <boost/spirit/home/x3/directive/lexeme.hpp>
#include <boost/spirit/home/x3/directive/with.hpp>
#include <boost/spirit/home/x3/nonterminal/rule.hpp>
#include <boost/spirit/home/x3/numeric/int.hpp>
#include <boost/spirit/home/x3/numeric/real.hpp>
#include <boost/spirit/home/x3/numeric/real_policies.hpp>
#include <boost/spirit/home/x3/operator/alternative.hpp>
#include <boost/spirit/home/x3/operator/kleene.hpp>
#include <boost/spirit/home/x3/operator/optional.hpp>
#include <boost/spirit/home/x3/operator/plus.hpp>
#include <boost/spirit/home/x3/operator/sequence.hpp>
#include <boost/spirit/home/x3/string/literal_string.hpp>
#include <boost/spirit/home/x3/string/symbols.hpp>

#include <cstddef>

namespace redfish::filter_grammar
{

// Tag for storing recursion depth in the parser context
struct RecursionDepth
{
    // Maximum recursion depth to prevent stack overflow
    constexpr static size_t max = 10;
};

// The below rules very intentionally use the same naming as section 7.3.4 of
// the redfish specification and are declared in the order of the precedence
// that the standard requires.

namespace details
{
using boost::spirit::x3::char_;
using boost::spirit::x3::int64;
using boost::spirit::x3::lexeme;
using boost::spirit::x3::lit;
using boost::spirit::x3::real_parser;
using boost::spirit::x3::rule;
using boost::spirit::x3::strict_real_policies;
using boost::spirit::x3::symbols;

using filter_ast::BooleanOp;
using filter_ast::Comparison;
using filter_ast::ComparisonOpEnum;
using filter_ast::LogicalAnd;
using filter_ast::LogicalNot;
using filter_ast::LogicalOr;
using filter_ast::QuotedString;
using filter_ast::UnquotedString;

// Clang format makes a mess of these rules and makes them hard to read
// clang-format on

// Basic argument types
const rule<class QuotedStringId, QuotedString> quotedString("QuotedString");
const rule<class UnquotedStrId, UnquotedString> unquotedString("UnquotedStr");

// Value comparisons -> boolean (value eq value) (value lt number)
const rule<class BooleanOpId, BooleanOp> booleanOp("BooleanOp");
const rule<class ComparisonId, Comparison> comparison("Comparison");

// Logical Comparisons (bool eq bool)
const rule<class LogicalAndId, LogicalAnd> logicalAnd("LogicalAnd");
const rule<class LogicalOrId, LogicalOr> logicalOr("LogicalOr");
const rule<class LogicalNotId, LogicalNot> logicalNot("LogicalNot");

///// BEGIN GRAMMAR

// Two types of strings.
const auto quotedString_def =
    '\'' >> lexeme[*('\\' >> char_ | ~char_('\''))] >> '\'';
const auto unquotedString_def = char_("a-zA-Z") >> *(char_("a-zA-Z0-9[]/"));

// Make sure we only parse true floating points as doubles
// This requires we have a "." which causes 1 to parse as int64, and 1.0 to
// parse as double
constexpr const real_parser<double, strict_real_policies<double>> strictDouble;

// Argument
const auto arg = strictDouble | int64 | unquotedString | quotedString;

// Greater Than/Less Than/Equals
const symbols<ComparisonOpEnum> compare{
    {"gt", ComparisonOpEnum::GreaterThan},
    {"ge", ComparisonOpEnum::GreaterThanOrEqual},
    {"lt", ComparisonOpEnum::LessThan},
    {"le", ComparisonOpEnum::LessThanOrEqual},
    {"ne", ComparisonOpEnum::NotEquals},
    {"eq", ComparisonOpEnum::Equals}};

// paren parser that limits the depth of the max recursion.
struct DepthCheckingParser : boost::spirit::x3::parser<DepthCheckingParser>
{
    using attribute_type = LogicalAnd;

    template <typename Iterator, typename Context, typename RContext,
              typename Attribute>
    bool parse(Iterator& first, const Iterator& last, const Context& context,
               RContext& rcontext, Attribute& attr) const
    {
        using boost::spirit::x3::get;
        using boost::spirit::x3::with;

        // Get current recursion depth from context
        auto depthRefWrapper = get<RecursionDepth>(context);
        unsigned int& currentDepth = depthRefWrapper.get();

        // Check if we've exceeded the maximum depth
        if (currentDepth >= RecursionDepth::max)
        {
            return false;
        }

        // Increment depth for this level
        ++currentDepth;

        // Parse with the current context (depth is already incremented via
        // reference)
        const auto parens = lit('(') >> logicalAnd >> lit(')');
        bool result = parens.parse(first, last, context, rcontext, attr);

        // Decrement depth after parsing
        --currentDepth;

        return result;
    }
};

// Parenthesis with depth checking
const DepthCheckingParser parens;

// Note, unlike most other comparisons, spaces are required here (one or more)
// to differentiate keys from values (ex Fooeq eq foo)
const auto comparison_def =
    lexeme[arg >> +lit(' ') >> compare >> +lit(' ') >> arg];

// Logical values
const auto booleanOp_def = comparison | parens;

// Not
const auto logicalNot_def = -(char_('n') >> lit("ot")) >> booleanOp;

// Or
const auto logicalOr_def = logicalNot >> *(lit("or") >> logicalNot);

// And
const auto logicalAnd_def = logicalOr >> *(lit("and") >> logicalOr);

BOOST_SPIRIT_DEFINE(booleanOp, logicalAnd, logicalNot, logicalOr, quotedString,
                    comparison, unquotedString);
///// END GRAMMAR

// Make the grammar and AST available outside of the system
static constexpr auto& grammar = logicalAnd;
using program = filter_ast::LogicalAnd;

} // namespace details

using details::grammar;
using details::program;

} // namespace redfish::filter_grammar
