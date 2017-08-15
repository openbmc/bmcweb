#pragma once
#include <dbus/connection.hpp>

namespace crow {
namespace connections {

static std::shared_ptr<dbus::connection> system_bus;

}  // namespace dbus
}  // namespace crow