#include "utils/time_utils.hpp"

#include "utils/extern/date.h"

#include <array>
#include <chrono>
#include <version>

namespace redfish::time_utils
{
using usSinceEpoch = std::chrono::duration<uint64_t, std::micro>;
std::optional<usSinceEpoch> dateStringToEpoch(std::string_view datetime)
{
    for (const char* format : std::to_array({"%FT%T%Ez", "%FT%TZ", "%FT%T"}))
    {
        // Parse using signed so we can detect negative dates
        std::chrono::sys_time<std::chrono::duration<int64_t, std::micro>> date;
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
            return usSinceEpoch{date.time_since_epoch().count()};
        }
    }
    return std::nullopt;
}
} // namespace redfish::time_utils
