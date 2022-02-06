#include "app.hpp"
#include "http_request.hpp"
#include "include/async_resp.hpp"
#include "nlohmann/json.hpp"
#include "redfish-core/lib/mock_sdbus.hpp"
#include "redfish-core/lib/telemetry_service.hpp"

#include <boost/shared_ptr.hpp>

#include <sstream>
#include <string>

#include "gmock/gmock.h"

TEST(TelemetryService, passing)
{
    EXPECT_EQ(0, 1);
    std::cerr << "started telem test" << std::endl;
    crow::connections::systemBus = std::make_shared<mockConnection>();
    const std::vector<std::pair<std::string, dbus::utility::DbusVariantType>>&
        arg;

    systemBus
        .unsafeMap[std::tuple<"/xyz/openbmc_project/Telemetry/Reports",
                              "org.freedesktop.DBus.Properties", "GetAll",
                              "xyz.openbmc_project.Telemetry.ReportManager">] =
        reinterpret_cast<void*>(arg);

    boost::beast::http::request<boost::beast::http::string_body> in;
    std::error_code ec;
    crow::Request req(in, ec);
    const std::shared_ptr<bmcweb::AsyncResp> shareAsyncResp =
        std::make_shared<bmcweb::AsyncResp>();

    shareAsyncResp->res.setCompleteRequestHandler(assertTelemetryService);

    redfish::handleTelemetryServiceGet(req, shareAsyncResp);
}

void assertTelemetryService(request req)
{
    std::cerr << "  assertTelemetryService " << std::endl;
}
