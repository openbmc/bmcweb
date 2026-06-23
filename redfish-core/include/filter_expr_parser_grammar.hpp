// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "filter_expr_parser_ast.hpp"

#include <boost/parser/parser.hpp>

#include <cstddef>
#include <cstdint>

namespace redfish::filter_grammar
{

namespace bp = boost::parser;

// Tag for storing recursion depth in the parser globals
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
using filter_ast::Argument;
using filter_ast::BooleanOp;
using filter_ast::Comparison;
using filter_ast::ComparisonOpEnum;
using filter_ast::LogicalAnd;
using filter_ast::LogicalNot;
using filter_ast::LogicalOr;
using filter_ast::QuotedString;
using filter_ast::UnquotedString;

// Boost.Parser has no named parser for int64_t, so construct one
constexpr bp::parser_interface<bp::int_parser<std::int64_t>> int64{};

// Basic argument types
bp::rule<struct QuotedStringId, QuotedString> const quotedString =
    "QuotedString";
bp::rule<struct UnquotedStrId, UnquotedString> const unquotedString =
    "UnquotedStr";
bp::rule<struct ArgumentId, Argument> const argument = "Argument";

// Value comparisons -> boolean (value eq value) (value lt number)
bp::rule<struct BooleanOpId, BooleanOp> const booleanOp = "BooleanOp";
bp::rule<struct ComparisonId, Comparison> const comparison = "Comparison";
bp::rule<struct ParensId, LogicalAnd> const parens = "Parens";

// Logical Comparisons (bool eq bool)
bp::rule<struct LogicalAndId, LogicalAnd> const logicalAnd = "LogicalAnd";
bp::rule<struct LogicalOrId, LogicalOr> const logicalOr = "LogicalOr";
bp::rule<struct LogicalNotId, LogicalNot> const logicalNot = "LogicalNot";

///// BEGIN GRAMMAR

// Two types of strings.  Both are only reached through comparison's lexeme[],
// so no skipper is active and quoted content keeps its spaces.
auto const quotedString_def =
    bp::lit('\'') >> *('\\' >> bp::char_ | (bp::char_ - '\'')) >> bp::lit('\'');
auto const unquotedString_def =
    (bp::char_('a', 'z') | bp::char_('A', 'Z')) >>
    *(bp::char_('a', 'z') | bp::char_('A', 'Z') | bp::char_('0', '9') |
      bp::char_("[]/"));

// Only match a double when a fractional or exponent part is present, so 1
// parses as int64 and 1.0 as double (bp::double_ alone parses "1" as 1.0).
auto const strictDouble =
    &(-bp::char_("+-") >> *bp::char_('0', '9') >> bp::char_(".eE")) >>
    bp::double_;

// Argument
auto const argument_def =
    strictDouble | int64 | unquotedString | quotedString;

// Greater Than/Less Than/Equals.  A plain alternation of literals is used
// instead of bp::symbols to avoid instantiating the (large) symbol trie.
auto const compare =
    (bp::lit("gt") >> bp::attr(ComparisonOpEnum::GreaterThan)) |
    (bp::lit("ge") >> bp::attr(ComparisonOpEnum::GreaterThanOrEqual)) |
    (bp::lit("lt") >> bp::attr(ComparisonOpEnum::LessThan)) |
    (bp::lit("le") >> bp::attr(ComparisonOpEnum::LessThanOrEqual)) |
    (bp::lit("ne") >> bp::attr(ComparisonOpEnum::NotEquals)) |
    (bp::lit("eq") >> bp::attr(ComparisonOpEnum::Equals));

// Limit recursion depth (threaded through the parse globals) to prevent stack
// overflow.
auto const incrementDepth = [](auto& ctx) {
    size_t& currentDepth = bp::_globals(ctx);
    if (currentDepth >= RecursionDepth::max)
    {
        bp::_pass(ctx) = false;
        return;
    }
    ++currentDepth;
};
auto const decrementDepth = [](auto& ctx) { --bp::_globals(ctx); };

// Parenthesis with depth checking
auto const parens_def = bp::lit('(') >> bp::eps[incrementDepth] >> logicalAnd >>
                        bp::eps[decrementDepth] >> bp::lit(')');

// Note, unlike most other comparisons, spaces are required here (one or more)
// to differentiate keys from values (ex Fooeq eq foo)
auto const comparison_def =
    bp::lexeme[argument >> +bp::lit(' ') >> compare >> +bp::lit(' ') >>
               argument];

// Logical values
auto const booleanOp_def = comparison | parens;

// Not
auto const logicalNot_def =
    -(bp::char_('n') >> bp::lit("ot")) >> booleanOp;

// Build first/rest explicitly; Boost.Parser would otherwise merge
// "x >> *(x)" into one container instead of the {first, rest} pair.
auto const assignOrFirst = [](auto& ctx) {
    bp::_val(ctx).first = bp::_attr(ctx);
};
auto const appendOrRest = [](auto& ctx) {
    bp::_val(ctx).rest.push_back(bp::_attr(ctx));
};
// Or
auto const logicalOr_def =
    logicalNot[assignOrFirst] >>
    *((bp::lit("or") >> logicalNot)[appendOrRest]);

auto const assignAndFirst = [](auto& ctx) {
    bp::_val(ctx).first = bp::_attr(ctx);
};
auto const appendAndRest = [](auto& ctx) {
    bp::_val(ctx).rest.push_back(bp::_attr(ctx));
};
// And
auto const logicalAnd_def =
    logicalOr[assignAndFirst] >>
    *((bp::lit("and") >> logicalOr)[appendAndRest]);

BOOST_PARSER_DEFINE_RULES(quotedString, unquotedString, argument, booleanOp,
                          comparison, parens, logicalAnd, logicalOr,
                          logicalNot);
///// END GRAMMAR

// Make the grammar and AST available outside of the system
constexpr auto& grammar = logicalAnd;
using program = filter_ast::LogicalAnd;

} // namespace details

using details::grammar;
using details::program;

} // namespace redfish::filter_grammar
