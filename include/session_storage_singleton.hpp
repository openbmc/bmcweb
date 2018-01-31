#pragma once
#include "sessions.hpp"

namespace crow {
namespace PersistentData {

static std::shared_ptr<SessionStore> session_store;

}  // namespace PersistentData
}  // namespace crow
