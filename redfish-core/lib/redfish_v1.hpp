#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "str_utility.hpp"
#include "utils/json_utils.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <array>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <ranges>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace redfish
{

void requestRoutesRedfish(App& app);

} // namespace redfish
