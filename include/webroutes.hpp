#pragma once

#include "flat_set.hpp"
namespace crow
{
namespace webroutes
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static bmcweb::FlatSet<std::string> routes;

} // namespace webroutes

} // namespace crow
