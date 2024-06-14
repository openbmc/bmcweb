#pragma once

#include "filter_expr_parser_ast.hpp"

#include <nlohmann/json.hpp>

namespace redfish
{

bool applyFilter(nlohmann::json& body,
                 const filter_ast::LogicalAnd& filterParam);

}
