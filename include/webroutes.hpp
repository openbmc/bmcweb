#pragma once

#include "flat_set.hpp"
namespace crow
{
namespace webroutes
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static crow::flat_set<std::string> routes;

} // namespace webroutes

} // namespace crow
