#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "query.hpp"
#include "redfish_aggregator.hpp"
#include "registries/privilege_registry.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

namespace redfish
{

void requestRoutesAggregationService(App& app);
void requestRoutesAggregationSourceCollection(App& app);
void requestRoutesAggregationSource(App& app);

} // namespace redfish
