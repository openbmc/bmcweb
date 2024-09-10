// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#include "utils/time_utils.hpp"

#include "utils/extern/date.h"

#include <array>
#include <chrono>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

namespace redfish::time_utils
{

std::optional<usSinceEpoch> dateStringToEpoch(std::string_view datetime)
{
    for (const char* format : std::to_array({"%FT%T%Ez", "%FT%TZ", "%FT%T"}))
    {
        // Parse using signed so we can detect negative dates
        std::chrono::sys_time<usSinceEpoch> date;
        std::istringstream iss(std::string{datetime});
#if __cpp_lib_chrono >= 201907L
        namespace chrono_from_stream = std::chrono;
#else
        namespace chrono_from_stream = date;
#endif
        if (chrono_from_stream::from_stream(iss, format, date))
        {
            if (date.time_since_epoch().count() < 0)
            {
                return std::nullopt;
            }
            if (iss.rdbuf()->in_avail() != 0)
            {
                // More information left at end of string.
                continue;
            }
            return date.time_since_epoch();
        }
    }
    return std::nullopt;
}
} // namespace redfish::time_utils
