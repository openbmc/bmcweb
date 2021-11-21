/*
// Copyright (c) 2019 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/
#pragma once

#include "async_resp_class_decl.hpp"
#include "dbus_utility.hpp"
#include "redfish_util.hpp"

#include <app_class_decl.hpp>

#include <variant>

namespace redfish
{
/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
void getIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& aResp);

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
// TODO (Gunnar): Remove IndicatorLED after enough time has passed
void setIndicatorLedState(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                         const std::string& ledState);

/**
 * @brief Retrieves identify led group properties over dbus
 *
 * @param[in] aResp     Shared pointer for generating response message.
 *
 * @return None.
 */
void getLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp);

/**
 * @brief Sets identify led group properties
 *
 * @param[in] aResp     Shared pointer for generating response message.
 * @param[in] ledState  LED state passed from request
 *
 * @return None.
 */
void setLocationIndicatorActive(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                               const bool ledState);

} // namespace redfish
