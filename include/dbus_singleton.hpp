#pragma once
#include <iostream>
#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/message/types.hpp>
#include <type_traits>

namespace crow
{
namespace connections
{
static std::shared_ptr<sdbusplus::asio::connection> systemBus;

} // namespace connections
} // namespace crow
