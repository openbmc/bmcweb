#include <boost/asio/impl/src.hpp>
#include <boost/assert/source_location.hpp>
#include <exception>

namespace boost {
void throw_exception(std::exception const&){
  std::terminate();
}

void throw_exception(std::exception const&, source_location const&){
  std::terminate();
}
}
