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
    "Copyright 2014-2018 DMTF in cooperation with the Storage Networking "
    "Industry Association (SNIA). All rights reserved.",
    "#MessageRegistry.v1_2_0.MessageRegistry",
    "TaskEvent.1.0.1",
    "Task Event Message Registry",
    "en",
    "This registry defines the messages for task related events.",
    "TaskEvent",
    "1.0.1",
    "DMTF",
};
constexpr std::array<MessageEntry, 9> registry = {
    MessageEntry{"TaskAborted",
                 {
                     "The task with id %1 has been aborted.",
                     "The task with id %1 has been aborted.",
                     "Critical",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCancelled",
                 {
                     "The task with id %1 has been cancelled.",
                     "The task with id %1 has been cancelled.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCompletedOK",
                 {
                     "The task with id %1 has completed.",
                     "The task with id %1 has completed.",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskCompletedWarning",
                 {
                     "The task with id %1 has completed with warnings.",
                     "The task with id %1 has completed with warnings.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskPaused",
                 {
                     "The task with id %1 has been paused.",
                     "The task with id %1 has been paused.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{
        "TaskProgressChanged",
        {
            "The task with id %1 has changed to progress %2 percent complete.",
            "The task with id %1 has changed to progress %2 percent complete.",
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
                     "The task with id %1 has been removed.",
                     "The task with id %1 has been removed.",
                     "Warning",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskResumed",
                 {
                     "The task with id %1 has been resumed.",
                     "The task with id %1 has been resumed.",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
    MessageEntry{"TaskStarted",
                 {
                     "The task with id %1 has started.",
                     "The task with id %1 has started.",
                     "OK",
                     1,
                     {
                         "string",
                     },
                     "None.",
                 }},
};
} // namespace redfish::message_registries::task_event
