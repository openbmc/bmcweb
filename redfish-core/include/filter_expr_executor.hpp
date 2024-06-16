#pragma once

#include "filter_expr_parser_ast.hpp"

#include <nlohmann/json.hpp>

namespace redfish
{

bool memberMatches(const nlohmann::json& member,
                   const filter_ast::LogicalAnd& filterParam);

bool applyFilterToCollection(nlohmann::json& body,
                             const filter_ast::LogicalAnd& filterParam);

} // namespace redfish
