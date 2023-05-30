#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>

#include <exception>

namespace boost
{
void throw_exception(const std::exception& /*e*/)
{
    std::terminate();
}

void throw_exception(const std::exception& /*e*/,
                     const source_location& /*loc*/)
{
    std::terminate();
}
} // namespace boost
