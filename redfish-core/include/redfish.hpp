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

    static OemRouter& getOemRouter()
    {
        static OemRouter instance;
        return instance;
    }

    static void oemRouterSetup()
    {
        getOemRouter().validate();
    }
};

template <const std::string_view& url>
constexpr auto& redfishOemRule()
{
    return RedfishService::getOemRouter()
        .template newOemRule<crow::utility::getParameterTag(url)>(
            std::string(url));
}

} // namespace redfish
