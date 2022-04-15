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

// Because some program elements can reference operand recursively, they need to
// be forward declared so that x3::forward_ast can be used
struct program;
struct negated;

// Represents a string that references an identifier
// (ie 'Enabled')
struct quoted_string : std::string
{};

// Represents a string that matches an identifier
struct unquoted_string : std::string
{};

// A struct that represents a key that can be operated on with arguments
// Note, we need to use the x3 variant here because this is recursive (ie it can contain itself)
struct operand :
    boost::spirit::x3::variant<unsigned int, quoted_string, unquoted_string,
                               boost::spirit::x3::forward_ast<negated>,
                               boost::spirit::x3::forward_ast<program>>
{
    using base_type::base_type;
    using base_type::operator=;
};

// An expression that has been negated with not()
struct negated
{
    char negateOp;
    operand operand_;
};

// An operation between two operands (for example and, or, gt, lt)
struct operation
{
    // Note, because the ast captures by "two character" operators, and
    // "and" is 3 characters, in the parser, we use lit() to "throw away" the
    // d, and turn the operator into "an".  This prevents us from having to
    // malloc strings for every operator, and works because there are no
    // other operators that start with "an" that would conflict.  All other
    // comparisons are two characters

    char operator1;
    char operator2;
    operand operand_;
};

// A list of expressions to execute
struct program
{
    operand first;
    std::list<operation> rest;
};
} // namespace filter_ast
} // namespace redfish

BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::operation, operator1, operator2,
                          operand_)

BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::negated, negateOp, operand_)

BOOST_FUSION_ADAPT_STRUCT(redfish::filter_ast::program, first, rest)
