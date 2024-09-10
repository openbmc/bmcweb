// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"

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
};

} // namespace redfish
