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
using boost::spirit::x3::double_;
using boost::spirit::x3::int64;
using boost::spirit::x3::lit;
using boost::spirit::x3::rule;

using filter_ast::BooleanOperation;
using filter_ast::comparisonOperator;
using filter_ast::LogicalAnd;
using filter_ast::LogicalNot;
using filter_ast::LogicalOr;
using filter_ast::QuotedString;
using filter_ast::RelationalComparison;
using filter_ast::UnquotedString;

// Clang format makes a mess of these rules and makes them hard to read
// clang-format off

// Basic argument types
rule<class quoted_string, QuotedString> const quotedString("quoted_string");
rule<class unquoted_string, UnquotedString> const unquotedString("unquoted_string");

// Value comparisons -> boolean (value eq value) (value lt number)
rule<class BooleanOperationId, BooleanOperation> const booleanOperation("BooleanOperation");
rule<class RelationalComparisonId, RelationalComparison> const relationalComparison("RelationalComparison");

// Logical Comparisons (bool eq bool)
rule<class LogicalAndId, LogicalAnd> const logicalAnd("LogicalAnd");
rule<class LogicalOrId, LogicalOr> const logicalOr("LogicalOr");
rule<class LogicalNotId, LogicalNot> const logicalNot("LogicalNot");

///// BEGIN GRAMMAR

const auto quotedString_def = '\'' >> *('\\' >> char_ | ~char_('\'')) >> '\'';
const auto unquotedString_def = char_("a-zA-Z") >> *(char_("a-zA-Z0-9[]/"));

// Spaces
// Filter examples have unclear guidelines about between which arguments spaces are
// Allowed or disallowed.  Specification is not clear, so in almost all cases we
// allow zero or more
const auto spaces = *lit(' ');

using boost::spirit::x3::real_parser;
using boost::spirit::x3::strict_real_policies;

// Make sure we only parse true floating points as doubles
// This requires we have a "." which causes 1 to parse as int64, and 1.0 to parse as float
constexpr const real_parser<double, strict_real_policies<double>> strictDouble;

// Argument
const auto argument =  strictDouble | int64 | unquotedString | quotedString;

// Greater Than/Less Than/Equals
// Note, unlike most other comparisons, spaces are required here (one or more) to differentiate
// keys from values (ex Fooeq eq foo)
const auto relationalComparison_def = argument >> +lit(' ') >>
                                        comparisonOperator() >> +lit(' ') >>
                                        argument;

// Parenthesis
const auto parens = lit('(') >> spaces >> logicalAnd >> spaces >> lit(')');

// Logical values
const auto booleanOperation_def = relationalComparison | parens;

// Not
const auto logicalNot_def = -(char_('n') >> lit("ot") >> spaces) >> booleanOperation;

// Or
const auto logicalOr_def = logicalNot >> *(spaces >> lit("or") >> spaces  >> logicalNot);

// And
const auto logicalAnd_def = logicalOr >> *(spaces  >> lit("and") >> spaces >> logicalOr);

BOOST_SPIRIT_DEFINE(
  booleanOperation,
  logicalAnd,
  logicalNot,
  logicalOr,
  quotedString,
  relationalComparison,
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
