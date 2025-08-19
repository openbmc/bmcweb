#pragma once
#include "account_service.hpp"
#include "app.hpp"
#include "async_resp.hpp"
#include "cookies.hpp"
#include "dbus_privileges.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "pam_authenticate.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "sessions.hpp"
#include "utils/json_utils.hpp"
#include <security/_pam_types.h>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace redfish
{

void requestRoutesSession(App& app);

} // namespace redfish