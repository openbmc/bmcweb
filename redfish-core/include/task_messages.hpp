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

namespace redfish
{

namespace messages
{

inline nlohmann::json taskAborted(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskAborted"},
        {"Message", "The task with id " + arg1 + " has been aborted."},
        {"MessageArgs", {arg1}},
        {"Severity", "Critical"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskCancelled(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskCancelled"},
        {"Message", "The task with id " + arg1 + " has been cancelled."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskCompletedOK(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskCompletedOK"},
        {"Message", "The task with id " + arg1 + " has Completed."},
        {"MessageArgs", {arg1}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskCompletedWarning(const std::string& arg1)
{
    return nlohmann::json{{"@odata.type", "#Message.v1_0_0.Message"},
                          {"MessageId", "TaskEvent.1.0.1.TaskCompletedWarning"},
                          {"Message", "The task with id " + arg1 +
                                          " has completed with warnings."},
                          {"MessageArgs", {arg1}},
                          {"Severity", "Warning"},
                          {"Resolution", "None."}};
}

inline nlohmann::json taskPaused(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskPaused"},
        {"Message", "The task with id " + arg1 + " has been paused."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskProgressChanged(const std::string& arg1,
                                          const size_t arg2)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskProgressChanged"},
        {"Message", "The task with id " + arg1 + " has changed to progress " +
                        std::to_string(arg2) + " percent complete."},
        {"MessageArgs", {arg1, std::to_string(arg2)}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskRemoved(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskRemoved"},
        {"Message", "The task with id " + arg1 + " has been removed."},
        {"MessageArgs", {arg1}},
        {"Severity", "Warning"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskResumed(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskResumed"},
        {"Message", "The task with id " + arg1 + " has been resumed."},
        {"MessageArgs", {arg1}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

inline nlohmann::json taskStarted(const std::string& arg1)
{
    return nlohmann::json{
        {"@odata.type", "#Message.v1_0_0.Message"},
        {"MessageId", "TaskEvent.1.0.1.TaskStarted"},
        {"Message", "The task with id " + arg1 + " has started."},
        {"MessageArgs", {arg1}},
        {"Severity", "OK"},
        {"Resolution", "None."}};
}

} // namespace messages

} // namespace redfish
