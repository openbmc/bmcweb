#include <http/http_request.hpp>
#include <http/http_response.hpp>

namespace redfish
{

bool redfishPreChecks(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string_view odataHeader = req.getHeaderValue("OData-Version");
    if (odataHeader.empty())
    {
        // Clients aren't required to provide odata version
        return true;
    }
    if (odataHeader != "4.0")
    {
        redfish::messages::preconditionFailed(asyncResp->res);
        return false;
    }

    asyncResp->res.addHeader("OData-Version", "4.0");
    return true;
}
} // namespace redfish