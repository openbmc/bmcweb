/*
// Copyright (c) 2018 Intel Corporation
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

#include <app_class_decl.hpp>
using crow::App;

namespace redfish
{

/**
 * SystemsCollection derived class for delivering ComputerSystems Collection
 * Schema
 */
void requestRoutesSystemsCollection(App& app);


/**
 * SystemActionsReset class supports handle POST method for Reset action.
 * The class retrieves and sends data directly to D-Bus.
 */
void requestRoutesSystemActionsReset(App& app);

/**
 * Systems derived class for delivering Computer Systems Schema.
 */
void requestRoutesSystems(App& app);

/**
 * SystemResetActionInfo derived class for delivering Computer Systems
 * ResetType AllowableValues using ResetInfo schema.
 */
void requestRoutesSystemResetActionInfo(App& app);

} // namespace redfish
