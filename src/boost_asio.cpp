// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "logging.hpp"

#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>

#include <exception>

namespace boost
{
void throw_exception(const std::exception& e)
{
    BMCWEB_LOG_CRITICAL("Boost exception thrown {}", e.what());
    std::terminate();
}

void throw_exception(const std::exception& e, const source_location& loc)
{
    BMCWEB_LOG_CRITICAL("Boost exception thrown {} from {}:{}", e.what(),
                        loc.file_name(), loc.line());
    std::terminate();
}
} // namespace boost
