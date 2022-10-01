#pragma once

#include <boost/unordered/unordered_flat_set.hpp>
namespace crow
{
namespace webroutes
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::unordered_flat_set<std::string> routes;

} // namespace webroutes

} // namespace crow
