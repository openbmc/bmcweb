#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>

#include <exception>

namespace boost
{
void throw_exception(const std::exception&)
{
    std::terminate();
}

void throw_exception(const std::exception&, const source_location&)
{
    std::terminate();
}
} // namespace boost
