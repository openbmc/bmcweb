#pragma once
#include <iostream>
#include <sdbusplus/asio/connection.hpp>

namespace mapbox
{
template <typename T, typename... Types>
const T* getPtr(const mapbox::util::variant<Types...>& v)
{
    if (v.template is<std::remove_const_t<T>>())
    {
        return &v.template get_unchecked<std::remove_const_t<T>>();
    }
    else
    {
        return nullptr;
    }
}
} // namespace mapbox

namespace crow
{
namespace connections
{
static std::shared_ptr<sdbusplus::asio::connection> systemBus;

} // namespace connections
} // namespace crow
