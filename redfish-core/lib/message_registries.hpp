#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "query.hpp"
#include "registries.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>

#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <ranges>
#include <utility>

namespace redfish
{

void requestRoutesMessageRegistryFileCollection(App& app);
void requestRoutesMessageRegistryFile(App& app);
void requestRoutesMessageRegistry(App& app);

} // namespace redfish
