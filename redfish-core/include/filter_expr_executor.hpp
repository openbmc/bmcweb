// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "filter_expr_parser_ast.hpp"

// NOLINTNEXTLINE(misc-include-cleaner)
#include <nlohmann/json.hpp>

namespace redfish
{

bool memberMatches(const nlohmann::json& member,
                   const filter_ast::LogicalAnd& filterParam);

bool applyFilterToCollection(nlohmann::json& body,
                             const filter_ast::LogicalAnd& filterParam);

} // namespace redfish
