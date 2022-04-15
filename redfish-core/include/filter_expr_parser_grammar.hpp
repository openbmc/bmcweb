#pragma once

#include <boost/spirit/home/x3.hpp>
#include <filter_expr_parser_ast.hpp>

namespace redfish::filter_grammar
{

namespace details
{
// The below rules very intentionally use the same naming as section 7.3.4 of
// the redfish specification and are declared in the order of the precedence
// that the standard requires

// clang-format off
using boost::spirit::x3::rule;
rule<class expression, filter_ast::operand> const expression("expression");
rule<class logical_negation, filter_ast::negated> const logical_negation("logical_negation");
rule<class relational_comparison, filter_ast::program> const relational_comparison("relational_comparison");
rule<class equality_comparison, filter_ast::program> const equality_comparison("equality_comparison");
rule<class logical_or, filter_ast::program> const logical_or("logical_or");
rule<class logical_and, filter_ast::program> const logical_and("logical_and");
rule<class quoted_string, filter_ast::quoted_string> const quoted_string("quoted_string");
rule<class unquoted_string, filter_ast::unquoted_string> const unquoted_string("unquoted_string");
// clang-format on

using boost::spirit::x3::char_;
using boost::spirit::x3::lit;
using boost::spirit::x3::uint_;

auto const quoted_string_def = '\'' >> *('\\' >> char_ | ~char_('\'')) >> '\'';

auto const unquoted_string_def = char_("a-zA-Z") >> *(char_("a-zA-Z0-9[]/"));

auto const basic_types = quoted_string | unquoted_string | uint_;

auto const logical_or_def = expression >>
                            *(char_('o') >> char_('r') >> expression);

auto const logical_and_def = logical_or >> *((char_('a') >> char_('n') >>
                                              lit('d') >> logical_or));

auto const
    equality_comparison_def = logical_and >>
                              *((char_('e') >> char_('q') >> logical_and) |
                                (char_('n') >> char_('e') >> logical_and));

auto const relational_comparison_def =
    equality_comparison >> *((char_('g') >> char_('t') >> equality_comparison) |
                             (char_('g') >> char_('e') >> equality_comparison) |
                             (char_('l') >> char_('t') >> equality_comparison) |
                             (char_('l') >> char_('e') >> equality_comparison));

auto const logical_negation_def = char_("n") >> lit("o") >>
                                  lit("t") >> expression;

auto const expression_def = '(' >> relational_comparison >> ')' |
                            logical_negation  |
                            basic_types;

BOOST_SPIRIT_DEFINE(logical_and, logical_or, equality_comparison, quoted_string, logical_negation,
                    unquoted_string, relational_comparison, expression);

auto grammar = relational_comparison;
} // namespace details

using details::grammar;

} // namespace redfish::filter_grammar
