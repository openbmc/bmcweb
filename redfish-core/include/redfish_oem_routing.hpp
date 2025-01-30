#pragma once

#include "async_resp.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "redfishoemrule.hpp"
#include "utils/error_code.hpp"
#include "utils/json_utils.hpp"
#include "utils/query_param.hpp"
#include "verb.hpp"

#include <nlohmann/json.hpp>

#include <memory>
#include <optional>
#include <string_view>
#include <vector>

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage, clang-diagnostic-unused-macros)
#define BMCWEB_OEM_ROUTE(router, url)                                          \
    router.template newRuleTagged<crow::utility::getParameterTag(url)>(url)

namespace redfish
{
template <template <typename...> class TaggedRuleT, typename BaseRuleT>    
class OemRouter : public crow::Router<TaggedRuleT, BaseRuleT> 
{
  public:
    OemRouter() = default;

    void handleOemGet(const std::shared_ptr<crow::Request>& /*req*/,
                      const std::shared_ptr<bmcweb::AsyncResp>& /*asyncResp*/) const
    {
        // BMCWEB_LOG_DEBUG("Checking OEM routes");
        // FindRouteResponse foundRoute = findRoute(*req);
        // std::vector<OemBaseRule*> fragments =
        //     std::move(foundRoute.route.fragmentRules);
        // std::vector<std::string> params = std::move(foundRoute.route.params);
        // if (!fragments.empty())
        // {
        //     std::function<void(crow::Response&)> handler =
        //         asyncResp->res.releaseCompleteRequestHandler();
        //     auto multiResp = std::make_shared<bmcweb::AsyncResp>();
        //     multiResp->res.setCompleteRequestHandler(std::move(handler));

        //     // Copy so that they exists when completion handler is called.
        //     auto uriFragments =
        //         std::make_shared<std::vector<OemBaseRule*>>(fragments);
        //     auto uriParams = std::make_shared<std::vector<std::string>>(params);

        //     asyncResp->res.setCompleteRequestHandler(std::bind_front(
        //         query_param::MultiAsyncResp::startMultiFragmentGet,
        //         std::make_shared<crow::Request>(*req), multiResp, uriFragments,
        //         uriParams));
        //}
        // else
        // {
        //     BMCWEB_LOG_DEBUG("No OEM routes found");
        // }
    }
};

using oemRouter = OemRouter<redfish::OemRule, redfish::OemBaseRule>;
inline oemRouter& getOemRouter()
{
    static oemRouter r;
    return r;
}

inline void oemRouterInit()
{
    getOemRouter().validate();
}

} // namespace redfish
