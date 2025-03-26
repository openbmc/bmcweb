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

    void validate()
    {
        // oemRouter.validate();
    }

    template <StringLiteral Rule>
    auto& newRoute(HttpVerb method)
    {
        return oemRouter.newRule<Rule>(method);
    }

    OemRouter oemRouter;
};

template <StringLiteral Path>
auto& REDFISH_SUB_ROUTE(RedfishService& service, HttpVerb method)
{
    return service.newRoute<Path>(method);
}

} // namespace redfish
