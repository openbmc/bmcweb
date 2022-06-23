#pragma once
#include <sdbusplus/asio/connection.hpp>

namespace crow
{
namespace connections
{
extern std::shared_ptr<sdbusplus::asio::connection> systemBus;

} // namespace connections
} // namespace crow
