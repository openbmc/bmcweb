// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include <boost/container/flat_set.hpp>
namespace crow
{
namespace webroutes
{

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_set<std::string> routes;

} // namespace webroutes

} // namespace crow
