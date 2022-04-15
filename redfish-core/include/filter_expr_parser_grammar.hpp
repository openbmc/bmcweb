#pragma once

#include "filter_expr_parser_ast.hpp"

#include <boost/spirit/home/x3.hpp>

namespace redfish::filter_grammar
{

// The below rules very intentionally use the same naming as section 7.3.4 of
// the redfish specification and are declared in the order of the precedence
// that the standard requires.

namespace details
{
using boost::spirit::x3::char_;
using boost::spirit::x3::int64;
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
// clang-format off

// Basic argument types
rule<class QuotedStringId, QuotedString> const quotedString("QuotedString");
rule<class UnQuotedStringId, UnquotedString> const unquotedString("UnQuotedString");

// Value comparisons -> boolean (value eq value) (value lt number)
rule<class BooleanOpId, BooleanOp> const booleanOp("BooleanOp");
rule<class ComparisonId, Comparison> const comparison("Comparison");

// Logical Comparisons (bool eq bool)
rule<class LogicalAndId, LogicalAnd> const logicalAnd("LogicalAnd");
rule<class LogicalOrId, LogicalOr> const logicalOr("LogicalOr");
rule<class LogicalNotId, LogicalNot> const logicalNot("LogicalNot");

///// BEGIN GRAMMAR

const auto quotedString_def = '\'' >> *('\\' >> char_ | ~char_('\'')) >> '\'';
const auto unquotedString_def = char_("a-zA-Z") >> *(char_("a-zA-Z0-9[]/"));

// Spaces
// Filter examples have unclear guidelines about between which arguments spaces
// are allowed or disallowed.  Specification is not clear, so in almost all
// cases we allow zero or more
const auto sp = *lit(' ');


// Make sure we only parse true floating points as doubles
// This requires we have a "." which causes 1 to parse as int64, and 1.0 to
// parse as float
constexpr const real_parser<double, strict_real_policies<double>> strictDouble;

// Argument
const auto arg =  strictDouble | int64 | unquotedString | quotedString;


// Note, unlike most other comparisons, spaces are required here (one or more)
// to differentiate keys from values (ex Fooeq eq foo)
const auto rsp = +lit(' ');

// Greater Than/Less Than/Equals
inline symbols<ComparisonOpEnum> compare()
{
    symbols<ComparisonOpEnum> ret;
    ret.add("gt", ComparisonOpEnum::GreaterThan);
    ret.add("ge", ComparisonOpEnum::GreaterThanOrEqual);
    ret.add("lt", ComparisonOpEnum::LessThan);
    ret.add("le", ComparisonOpEnum::LessThanOrEqual);
    ret.add("ne", ComparisonOpEnum::NotEquals);
    ret.add("eq", ComparisonOpEnum::Equals);
    return ret;
}

const auto comparison_def = arg >> rsp >> compare() >> rsp >> arg;

// Parenthesis
const auto parens = lit('(') >> sp >> logicalAnd >> sp >> lit(')');

// Logical values
const auto booleanOp_def = comparison | parens;

// Not
const auto logicalNot_def = -(char_('n') >> lit("ot") >> sp) >> booleanOp;

// Or
const auto logicalOr_def = logicalNot >> *(sp >> lit("or") >> sp  >> logicalNot);

// And
const auto logicalAnd_def = logicalOr >> *(sp  >> lit("and") >> sp >> logicalOr);

BOOST_SPIRIT_DEFINE(
  booleanOp,
  logicalAnd,
  logicalNot,
  logicalOr,
  quotedString,
  comparison,
  unquotedString
);
///// END GRAMMAR

// clang-format on

// Make the grammar and AST available outside of the system
static constexpr auto& grammar = logicalAnd;
using program = filter_ast::LogicalAnd;

} // namespace details

using details::grammar;
using details::program;

} // namespace redfish::filter_grammar
