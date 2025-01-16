#pragma once

#include "task.hpp"

#include <boost/system/error_code.hpp>
#include <sdbusplus/message.hpp>

#include <memory>

namespace redfish
{

inline bool handleTaskMessage(const boost::system::error_code& ec,
                              sdbusplus::message_t& msg,
                              const std::shared_ptr<task::TaskData>& taskData)
{
    if (ec)
    {
        taskData->messages.emplace_back(messages::internalError());
        return task::completed;
    }

    std::string iface;
    dbus::utility::DBusPropertiesMap values;

    std::string index = std::to_string(taskData->index);
    msg.read(iface, values);

    const std::string* state = nullptr;
    const std::string* status = nullptr;
    const uint8_t* progress = nullptr;

    if (!sdbusplus::unpackPropertiesNoThrow(
            redfish::dbus_utils::UnpackErrorPrinter(), values, //
            "Activation", state,  // Software.Activation
            "Progress", progress, // Software.ActivationProgress
            "Status", status      // Common.Progress
            ))
    {
        return !task::completed;
    }

    if (state != nullptr)
    {
        if (*state == "xyz.openbmc_project.Software.Activation.Invalid" ||
            *state == "xyz.openbmc_project.Software.Activation.Failed")
        {
            taskData->state = "Exception";
            taskData->status = "Warning";
            taskData->messages.emplace_back(messages::taskAborted(index));
            return task::completed;
        }
        if (*state == "xyz.openbmc_project.Software.Activation.Staged")
        {
            taskData->state = "Stopping";
            taskData->messages.emplace_back(messages::taskPaused(index));

            // its staged, set a long timer to allow them time to complete the
            // update (probably cycle the system) if this expires then task will
            // be canceled
            taskData->extendTimer(std::chrono::hours(5));
            return !task::completed;
        }
        else if (*state == "xyz.openbmc_project.Software.Activation.Active")
        {
            taskData->messages.emplace_back(messages::taskCompletedOK(index));
            taskData->state = "Completed";
            return task::completed;
        }
    }

    if (progress != nullptr)
    {
        taskData->percentComplete = *progress;
        taskData->messages.emplace_back(
            messages::taskProgressChanged(index, *progress));

        // if we're getting status updates it's
        // still alive, update timer
        taskData->extendTimer(std::chrono::minutes(5));
    }

    if (status != nullptr)
    {
        if (*status ==
            "xyz.openbmc_project.Common.Progress.OperationStatus.InProgress")
        {
            taskData->extendTimer(std::chrono::minutes(5));
        }
        else if (
            *status ==
            "xyz.openbmc_project.Common.Progress.OperationStatus.Completed")
        {
            taskData->messages.emplace_back(messages::taskCompletedOK(index));
            taskData->state = "Completed";
            return task::completed;
        }
        else if (
            *status ==
                "xyz.openbmc_project.Common.Progress.OperationStatus.Failed" ||
            *status ==
                "xyz.openbmc_project.Common.Progress.OperationStatus.Aborted")
        {
            taskData->state = "Exception";
            taskData->status = "Warning";
            taskData->messages.emplace_back(messages::taskAborted(index));
            return task::completed;
        }
    }

    // as firmware update often results in a
    // reboot, the task  may never "complete"
    // unless it is an error

    return !task::completed;
}

inline void createTaskForDbusPath(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    task::Payload&& payload, const sdbusplus::message::object_path& objPath)
{
    std::shared_ptr<task::TaskData> task = task::TaskData::createTask(
        handleTaskMessage,
        "type='signal',interface='org.freedesktop.DBus.Properties',"
        "member='PropertiesChanged',path='" +
            objPath.str + "'");
    task->startTimer(std::chrono::minutes(5));
    task->populateResp(asyncResp->res);
    task->payload.emplace(std::move(payload));
}

} // namespace redfish
