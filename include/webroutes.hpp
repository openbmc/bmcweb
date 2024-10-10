#pragma once

#include <boost/container/flat_set.hpp>
namespace bmcweb
{
namespace webroutes
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_set<std::string> routes;

} // namespace webroutes

} // namespace bmcweb

namespace crow = bmcweb;
