#pragma once
#include "bmcweb_config.h"

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_singleton.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "http/parsing.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "io_context_singleton.hpp"
#include "logging.hpp"
#include "privileges.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "utility.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/time_utils.hpp"

#include <systemd/sd-bus.h>

#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/result.hpp>
#include <boost/url/format.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iterator>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace redfish
{

void getCertificateProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& objectPath, const std::string& service,
    const std::string& certId, const boost::urls::url& certURL,
    const std::string& name);
void requestRoutesCertificateService(App& app);
void requestRoutesHTTPSCertificate(App& app);
void requestRoutesLDAPCertificate(App& app);
void requestRoutesTrustStoreCertificate(App& app);

} // namespace redfish
