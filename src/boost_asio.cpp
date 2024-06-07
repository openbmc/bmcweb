#include "logging.hpp"
#include "stacktrace.hpp"

#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>

#include <exception>

namespace boost
{
void throw_exception(const std::exception& e)
{
    BMCWEB_LOG_CRITICAL("Boost exception thrown {}", e.what());
    print_stacktrace();
    std::abort();
}

void throw_exception(const std::exception& e, const source_location& loc)
{
    BMCWEB_LOG_CRITICAL("Boost exception thrown {} from {}:{}", e.what(),
                        loc.file_name(), loc.line());
    print_stacktrace();
    std::abort();
}
} // namespace boost
