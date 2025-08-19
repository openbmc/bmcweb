#pragma once
#include "filter_expr_parser_ast.hpp"
#include "filter_expr_printer.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include "registries/privilege_registry.hpp"
#include "server_sent_event.hpp"
#include "subscription.hpp"

#include <app.hpp>
#include <boost/url/params_base.hpp>
#include <event_service_manager.hpp>

#include <format>
#include <memory>
#include <optional>
#include <string>

namespace redfish
{

void requestRoutesEventServiceSse(App& app);

} // namespace redfish
