/*
// Copyright (c) 2020 Intel Corporation
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
/****************************************************************
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 ***************************************************************/
#pragma once
#include <registries.hpp>

namespace redfish::message_registries::task_event
{
const Header header = {
    "Copyright 2014-2020 DMTF in cooperation with the Storage Networking "
    "Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_4_0.MessageRegistry",
    "TaskEvent.1.0.2",
    "Task Event Message Registry",
    "en",
    "This registry defines the messages for task related events.",
    "TaskEvent",
    "1.0.2",
    "DMTF",
};
constexpr const char* url =
    "https://redfish.dmtf.org/registries/TaskEvent.1.0.2.json";

constexpr std::array<MessageEntry, 9> registry = {
    MessageEntry{"TaskAborted",
                 {
                     "A task has been aborted.",
                     "The task with Id '%1' has been aborted.",
                     "Critical",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCancelled",
                 {
                     "A task has been cancelled.",
                     "The task with Id '%1' has been cancelled.",
                     "Warning",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCompletedOK",
                 {
                     "A task has completed.",
                     "The task with Id '%1' has completed.",
                     "OK",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCompletedWarning",
                 {
                     "A task has completed with warnings.",
                     "The task with Id '%1' has completed with warnings.",
                     "Warning",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskPaused",
                 {
                     "A task has been paused.",
                     "The task with Id '%1' has been paused.",
                     "Warning",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskProgressChanged",
                 {
                     "A task has changed progress.",
                     "The task with Id '%1' has changed to progress %2 percent "
                     "complete.",
                     "OK",
                     "OK",
                     2,
                     {
                         "string",
                         "number",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskRemoved",
                 {
                     "A task has been removed.",
                     "The task with Id '%1' has been removed.",
                     "Warning",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskResumed",
                 {
                     "A task has been resumed.",
                     "The task with Id '%1' has been resumed.",
                     "OK",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskStarted",
                 {
                     "A task has started.",
                     "The task with Id '%1' has started.",
                     "OK",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
};
} // namespace redfish::message_registries::task_event
