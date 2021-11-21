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

#include "health_class_decl.hpp"

#include <app_class_decl.hpp>
#include <boost/container/flat_map.hpp>
#include <registries/privilege_registry.hpp>
#include <utils/collection.hpp>
#include <utils/hex_utils.hpp>
#include <utils/json_utils.hpp>

using crow::App;

namespace redfish
{

using DimmProperty =
    std::variant<std::string, std::vector<uint32_t>, std::vector<uint16_t>,
                 uint64_t, uint32_t, uint16_t, uint8_t, bool>;

using DimmProperties = boost::container::flat_map<std::string, DimmProperty>;

std::string translateMemoryTypeToRedfish(const std::string& memoryType);

void dimmPropToHex(const std::shared_ptr<bmcweb::AsyncResp>& aResp,
                          const char* key,
                          const std::pair<std::string, DimmProperty>& property);

void getPersistentMemoryProperties(
    const std::shared_ptr<bmcweb::AsyncResp>& aResp,
    const DimmProperties& properties);

void getDimmDataByService(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& dimmId,
                                 const std::string& service,
                                 const std::string& objPath);

void getDimmPartitionData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                                 const std::string& service,
                                 const std::string& path);

void getDimmData(std::shared_ptr<bmcweb::AsyncResp> aResp,
                        const std::string& dimmId);

void requestRoutesMemoryCollection(App& app);

void requestRoutesMemory(App& app);

} // namespace redfish
