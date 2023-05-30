#include "logging.hpp"

#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>

#include <exception>

namespace boost
{
void throw_exception(const std::exception& /*e*/)
{
    BMCWEB_LOG_DEBUG << "Boost exception thrown " << e.message();
    std::terminate();
}

void throw_exception(const std::exception& /*e*/,
                     const source_location& /*loc*/)
{
    BMCWEB_LOG_DEBUG << "Boost exception thrown " << e.message() << " from "
                     << loc;
    std::terminate();
}
} // namespace boost
