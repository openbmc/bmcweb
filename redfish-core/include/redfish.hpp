// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "redfish_oem_routing.hpp"
#include "utility.hpp"

#include <string>
#include <string_view>

namespace redfish
{
/*
 * @brief Top level class installing and providing Redfish services
 */
class RedfishService
{
  public:
    /*
     * @brief Redfish service constructor
     *
     * Loads Redfish configuration and installs schema resources
     *
     * @param[in] app   Crow app on which Redfish will initialize
     */
    explicit RedfishService(App& app);

    // Temporary change to make redfish instance available in other places
    // like query delegation.
    static RedfishService& getInstance(App& app)
    {
        static RedfishService redfish(app);
        return redfish;
    }

    void validate()
    {
        oemRouter.validate();
    }

    template <StringLiteral Rule>
    auto& newRoute(HttpVerb method)
    {
        return oemRouter.newRule<Rule>(method);
    }

    void handleSubRoute(
        const crow::Request& req,
        const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) const
    {
        oemRouter.handle(req, asyncResp);
    }

    OemRouter oemRouter;
};

template <StringLiteral Path>
auto& REDFISH_SUB_ROUTE(RedfishService& service, HttpVerb method)
{
    return service.newRoute<Path>(method);
}

} // namespace redfish
