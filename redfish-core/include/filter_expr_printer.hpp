#pragma once

#include "filter_expr_parser_ast.hpp"

#include <memory>
#include <optional>
#include <string_view>

namespace redfish
{
struct FilterExpressionPrinter
{
    using result_type = std::string;
    std::string operator()(double x) const;
    std::string operator()(int64_t x) const;
    std::string operator()(const filter_ast::QuotedString& x) const;
    std::string operator()(const filter_ast::UnquotedString& x) const;
    std::string operator()(const filter_ast::LogicalNot& x) const;
    std::string operator()(const filter_ast::LogicalOr& x) const;
    std::string operator()(const filter_ast::LogicalAnd& x) const;
    std::string operator()(const filter_ast::Comparison& x) const;
    std::string operator()(const filter_ast::BooleanOp& operation) const;
};

std::optional<filter_ast::LogicalAnd> parseFilter(std::string_view expr);
} // namespace redfish
