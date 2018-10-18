#pragma once
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/message/types.hpp>
#include <type_traits>

namespace mapbox
{
template <typename T, typename... Types>
const T* getPtr(const sdbusplus::message::variant<Types...>& v)
{
    namespace variant_ns = sdbusplus::message::variant_ns;
    return variant_ns::get_if<std::remove_const_t<T>, Types...>(&v);
}
} // namespace mapbox

namespace crow
{
namespace connections
{
static std::shared_ptr<sdbusplus::asio::connection> systemBus;

} // namespace connections
} // namespace crow
