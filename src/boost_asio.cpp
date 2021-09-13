#include <boost/asio/impl/src.hpp>

namespace boost
{
void throw_exception(std::exception const&)
{
    std::terminate();
}

void throw_exception(std::exception const&, boost::source_location const&)
{
    std::terminate();
}
} // namespace boost