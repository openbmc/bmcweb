#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"

#include <boost/beast/http/verb.hpp>
#include <boost/url/format.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

void redfishOdataGet(const crow::Request& /*req*/,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);
void requestRoutesOdata(App& app);

} // namespace redfish
