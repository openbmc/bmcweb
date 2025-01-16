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

#include "bmcweb_config.h"

#include "app.hpp"
#include "dbus_utility.hpp"
#include "error_messages.hpp"
#include "generated/enums/update_service.hpp"
#include "multipart_parser.hpp"
#include "ossl_random.hpp"
#include "query.hpp"
#include "registries/privilege_registry.hpp"
#include "task.hpp"
#include "task_messages.hpp"
#include "utils/collection.hpp"
#include "utils/dbus_utils.hpp"
#include "utils/json_utils.hpp"
#include "utils/sw_utils.hpp"

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/range/adaptor/reversed.hpp>
#include <boost/system/error_code.hpp>
#include <boost/url/format.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace redfish
{

// Match signals added on software path
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::unique_ptr<sdbusplus::bus::match_t> fwUpdateMatcher;
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::unique_ptr<sdbusplus::bus::match_t> fwUpdateErrorMatcher;
// Only allow one update at a time
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static bool fwUpdateInProgress = false;
// Timer for software available
// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
static std::unique_ptr<boost::asio::steady_timer> fwAvailableTimer;

inline void cleanUp()
{
    fwUpdateInProgress = false;
    fwUpdateMatcher = nullptr;
    fwUpdateErrorMatcher = nullptr;
}

inline void activateImage(const std::string& objPath,
                          const std::string& service)
{
    BMCWEB_LOG_DEBUG("Activate image for {} {}", objPath, service);
    sdbusplus::asio::setProperty(
        *crow::connections::systemBus, service, objPath,
        "xyz.openbmc_project.Software.Activation", "RequestedActivation",
        "xyz.openbmc_project.Software.Activation.RequestedActivations.Active",
        [](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_DEBUG("error_code = {}", ec);
                BMCWEB_LOG_DEBUG("error msg = {}", ec.message());
            }
        });
}

// Return true if Errror LoggingEntry handling is done
inline bool doUpdateErrorLoggingEntry(
    const std::shared_ptr<task::TaskData>& taskData, const std::string& index,
    const dbus::utility::DBusPropertiesMap& properties)
{
    using AdditionalDataType = std::vector<std::string>;

    const AdditionalDataType* addData = nullptr;
    const std::string* message = nullptr;
    const std::string* eventId = nullptr;

    const bool success = sdbusplus::unpackPropertiesNoThrow(
        dbus_utils::UnpackErrorPrinter(), properties, "Message", message,
        "AdditionalData", addData, "EventId", eventId);
    if (!success)
    {
        BMCWEB_LOG_ERROR("Log property has unexpected value");
        taskData->messages.emplace_back(messages::internalError());
        return true;
    }
    if (message == nullptr)
    {
        BMCWEB_LOG_ERROR("Log property has unexpected value");
        taskData->messages.emplace_back(messages::internalError());
        return true;
    }

    if (!message->starts_with("xyz.openbmc_project.Software"))
    {
        // Not a software error
        return false;
    }

    if (addData == nullptr)
    {
        BMCWEB_LOG_ERROR("Log property has unexpected value");
        taskData->messages.emplace_back(messages::internalError());
        return true;
    }
    if (eventId == nullptr)
    {
        BMCWEB_LOG_ERROR("Log property has unexpected value");
        taskData->messages.emplace_back(messages::internalError());
        return true;
    }

    std::string addDataStr;
    for (const auto& data : *addData)
    {
        addDataStr.append(data);
        addDataStr.append(" ");
    }

    if (!message->empty() && !addDataStr.empty() && !eventId->empty())
    {
        taskData->messages.emplace_back(
            messages::taskAborted(index, *message, addDataStr, *eventId));
        return true;
    }
    return false;
}

inline void afterUpdateErrorLogMessage(
    const std::shared_ptr<task::TaskData>& taskData, const std::string& index,
    const boost::system::error_code& ec,
    const dbus::utility::ManagedObjectType& resp)
{
    if (ec)
    {
        BMCWEB_LOG_ERROR("updateErrorLogMessage returned an error {}",
                         ec.value());
        return;
    }

    for (const auto& objectPath : boost::adaptors::reverse(resp))
    {
        for (const auto& interfaceMap : objectPath.second)
        {
            if (interfaceMap.first == "xyz.openbmc_project.Logging.Entry")
            {
                if (doUpdateErrorLoggingEntry(taskData, index,
                                              interfaceMap.second))
                {
                    return;
                }
            }
        }
    }
}

inline void updateErrorLogMessage(
    const std::shared_ptr<task::TaskData>& taskData, const std::string& index)
{
    sdbusplus::message::object_path path("/xyz/openbmc_project/logging");

    dbus::utility::getManagedObjects(
        "xyz.openbmc_project.Logging", path,
        std::bind_front(afterUpdateErrorLogMessage, taskData, index));
}

// Note that asyncResp can be either a valid pointer or nullptr. If nullptr
// then no asyncResp updates will occur
static void
    softwareInterfaceAdded(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           sdbusplus::message_t& m, task::Payload&& payload)
{
    dbus::utility::DBusInterfacesMap interfacesProperties;

    sdbusplus::message::object_path objPath;

    m.read(objPath, interfacesProperties);

    BMCWEB_LOG_DEBUG("obj path = {}", objPath.str);
    for (const auto& interface : interfacesProperties)
    {
        BMCWEB_LOG_DEBUG("interface = {}", interface.first);

        if (interface.first == "xyz.openbmc_project.Software.Activation")
        {
            // Retrieve service and activate
            constexpr std::array<std::string_view, 1> interfaces = {
                "xyz.openbmc_project.Software.Activation"};
            dbus::utility::getDbusObject(
                objPath.str, interfaces,
                [objPath, asyncResp, payload(std::move(payload))](
                    const boost::system::error_code& ec,
                    const std::vector<
                        std::pair<std::string, std::vector<std::string>>>&
                        objInfo) mutable {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG("error_code = {}", ec);
                        BMCWEB_LOG_DEBUG("error msg = {}", ec.message());
                        if (asyncResp)
                        {
                            messages::internalError(asyncResp->res);
                        }
                        cleanUp();
                        return;
                    }
                    // Ensure we only got one service back
                    if (objInfo.size() != 1)
                    {
                        BMCWEB_LOG_ERROR("Invalid Object Size {}",
                                         objInfo.size());
                        if (asyncResp)
                        {
                            messages::internalError(asyncResp->res);
                        }
                        cleanUp();
                        return;
                    }
                    // cancel timer only when
                    // xyz.openbmc_project.Software.Activation interface
                    // is added
                    fwAvailableTimer = nullptr;

                    activateImage(objPath.str, objInfo[0].first);
                    if (asyncResp)
                    {
                        std::shared_ptr<task::TaskData> task =
                            task::TaskData::createTask(
                                [](const boost::system::error_code& ec2,
                                   sdbusplus::message_t& msg,
                                   const std::shared_ptr<task::TaskData>&
                                       taskData) {
                                    if (ec2)
                                    {
                                        return task::completed;
                                    }

                                    std::string iface;
                                    dbus::utility::DBusPropertiesMap values;

                                    std::string index =
                                        std::to_string(taskData->index);
                                    msg.read(iface, values);

                                    if (iface ==
                                        "xyz.openbmc_project.Software.Activation")
                                    {
                                        const std::string* state = nullptr;
                                        for (const auto& property : values)
                                        {
                                            if (property.first == "Activation")
                                            {
                                                state =
                                                    std::get_if<std::string>(
                                                        &property.second);
                                                if (state == nullptr)
                                                {
                                                    taskData->messages
                                                        .emplace_back(
                                                            messages::
                                                                internalError());
                                                    return task::completed;
                                                }
                                            }
                                        }

                                        if (state == nullptr)
                                        {
                                            return !task::completed;
                                        }

                                        if (state->ends_with("Invalid") ||
                                            state->ends_with("Failed"))
                                        {
                                            taskData->state = "Exception";
                                            taskData->status = "Warning";
                                            updateErrorLogMessage(taskData,
                                                                  index);
                                            return task::completed;
                                        }

                                        if (state->ends_with("Staged"))
                                        {
                                            taskData->state = "Stopping";
                                            taskData->messages.emplace_back(
                                                messages::taskPaused(index));

                                            // its staged, set a long timer to
                                            // allow them time to complete the
                                            // update (probably cycle the
                                            // system) if this expires then
                                            // task will be canceled
                                            taskData->extendTimer(
                                                std::chrono::hours(5));
                                            return !task::completed;
                                        }

                                        if (state->ends_with("Active"))
                                        {
                                            taskData->messages.emplace_back(
                                                messages::taskCompletedOK(
                                                    index));
                                            taskData->state = "Completed";
                                            return task::completed;
                                        }
                                    }
                                    else if (
                                        iface ==
                                        "xyz.openbmc_project.Software.ActivationProgress")
                                    {
                                        const uint8_t* progress = nullptr;
                                        for (const auto& property : values)
                                        {
                                            if (property.first == "Progress")
                                            {
                                                progress = std::get_if<uint8_t>(
                                                    &property.second);
                                                if (progress == nullptr)
                                                {
                                                    taskData->messages
                                                        .emplace_back(
                                                            messages::
                                                                internalError());
                                                    return task::completed;
                                                }
                                            }
                                        }

                                        if (progress == nullptr)
                                        {
                                            return !task::completed;
                                        }
                                        taskData->percentComplete = *progress;
                                        taskData->messages.emplace_back(
                                            messages::taskProgressChanged(
                                                index, *progress));

                                        // if we're getting status updates it's
                                        // still alive, update timer
                                        taskData->extendTimer(
                                            std::chrono::minutes(5));
                                    }

                                    // as firmware update often results in a
                                    // reboot, the task  may never "complete"
                                    // unless it is an error

                                    return !task::completed;
                                },
                                "type='signal',interface='org.freedesktop.DBus.Properties',"
                                "member='PropertiesChanged',path='" +
                                    objPath.str + "'");
                        task->startTimer(std::chrono::minutes(5));
                        task->populateResp(asyncResp->res);
                        task->payload.emplace(std::move(payload));
                    }
                    fwUpdateInProgress = false;
                });

            break;
        }
    }
}

// TODO(Gunnar): Remove this 4/1/25
inline void createBMCDump()
{
    std::vector<std::pair<std::string, std::variant<std::string, uint64_t>>>
        createDumpParamVec;

    createDumpParamVec.emplace_back(
        "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorId",
        "bmcweb-internal");
    createDumpParamVec.emplace_back(
        "xyz.openbmc_project.Dump.Create.CreateParameters.OriginatorType",
        "xyz.openbmc_project.Common.OriginatedBy.OriginatorTypes.Internal");

    crow::connections::systemBus->async_method_call(
        [](const boost::system::error_code& ec) {
            if (ec)
            {
                BMCWEB_LOG_ERROR("Failed to do a dump:{}", ec.value());
                return;
            }
        },
        "xyz.openbmc_project.Dump.Manager", "/xyz/openbmc_project/dump/bmc",
        "xyz.openbmc_project.Dump.Create", "CreateDump", createDumpParamVec);
}

inline void afterAvailbleTimerAsyncWait(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec)
{
    cleanUp();
    if (ec == boost::asio::error::operation_aborted)
    {
        // expected, we were canceled before the timer completed.
        return;
    }
    BMCWEB_LOG_ERROR("Timed out waiting for firmware object being created");
    BMCWEB_LOG_ERROR("FW image may has already been uploaded to server");
    // TODO(Gunnar): Remove this 4/1/25, this is here to try to figure what is
    // slowing down the system so much.
    createBMCDump();
    if (ec)
    {
        BMCWEB_LOG_ERROR("Async_wait failed{}", ec);
        return;
    }
    if (asyncResp)
    {
        redfish::messages::internalError(asyncResp->res);
    }
}

inline void
    handleUpdateErrorType(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& url, const std::string& type)
{
    if (type == "xyz.openbmc_project.Software.Image.Error.UnTarFailure")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Invalid archive");
    }
    else if (type ==
             "xyz.openbmc_project.Software.Image.Error.ManifestFileFailure")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Invalid manifest");
    }
    else if (type == "xyz.openbmc_project.Software.Image.Error.ImageFailure")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Invalid image format");
    }
    else if (type == "xyz.openbmc_project.Software.Version.Error.AlreadyExists")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Image version already exists");

        redfish::messages::resourceAlreadyExists(
            asyncResp->res, "UpdateService", "Version", "uploaded version");
    }
    else if (type == "xyz.openbmc_project.Software.Image.Error.BusyFailure")
    {
        redfish::messages::resourceExhaustion(asyncResp->res, url);
    }
    else if (type == "xyz.openbmc_project.Software.Version.Error.Incompatible")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Incompatible image version");
    }
    else if (type ==
             "xyz.openbmc_project.Software.Version.Error.ExpiredAccessKey")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Update Access Key Expired");
    }
    else if (type ==
             "xyz.openbmc_project.Software.Version.Error.InvalidSignature")
    {
        redfish::messages::invalidUpload(asyncResp->res, url,
                                         "Invalid image signature");
    }
    else if (type ==
                 "xyz.openbmc_project.Software.Image.Error.InternalFailure" ||
             type == "xyz.openbmc_project.Software.Version.Error.HostFile")
    {
        BMCWEB_LOG_ERROR("Software Image Error type={}", type);
        redfish::messages::internalError(asyncResp->res);
    }
    else
    {
        // Unrelated error types. Ignored
        BMCWEB_LOG_INFO("Non-Software-related Error type={}. Ignored", type);
        return;
    }
    // Clear the timer
    fwAvailableTimer = nullptr;
}

inline void
    afterUpdateErrorMatcher(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& url, sdbusplus::message_t& m)
{
    dbus::utility::DBusInterfacesMap interfacesProperties;
    sdbusplus::message::object_path objPath;
    m.read(objPath, interfacesProperties);
    BMCWEB_LOG_DEBUG("obj path = {}", objPath.str);
    for (const std::pair<std::string, dbus::utility::DBusPropertiesMap>&
             interface : interfacesProperties)
    {
        if (interface.first == "xyz.openbmc_project.Logging.Entry")
        {
            for (const std::pair<std::string, dbus::utility::DbusVariantType>&
                     value : interface.second)
            {
                if (value.first != "Message")
                {
                    continue;
                }
                const std::string* type =
                    std::get_if<std::string>(&value.second);
                if (type == nullptr)
                {
                    // if this was our message, timeout will cover it
                    return;
                }
                handleUpdateErrorType(asyncResp, url, *type);
            }
        }
    }
}

// Note that asyncResp can be either a valid pointer or nullptr. If nullptr
// then no asyncResp updates will occur
static void monitorForSoftwareAvailable(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const crow::Request& req, const std::string& url,
    int timeoutTimeSeconds = 25)
{
    // Only allow one FW update at a time
    if (fwUpdateInProgress)
    {
        if (asyncResp)
        {
            messages::serviceTemporarilyUnavailable(asyncResp->res, "30");
        }
        return;
    }

    if (req.ioService == nullptr)
    {
        messages::internalError(asyncResp->res);
        return;
    }

    fwAvailableTimer =
        std::make_unique<boost::asio::steady_timer>(*req.ioService);

    fwAvailableTimer->expires_after(std::chrono::seconds(timeoutTimeSeconds));

    fwAvailableTimer->async_wait(
        std::bind_front(afterAvailbleTimerAsyncWait, asyncResp));

    task::Payload payload(req);
    auto callback = [asyncResp, payload](sdbusplus::message_t& m) mutable {
        BMCWEB_LOG_DEBUG("Match fired");
        softwareInterfaceAdded(asyncResp, m, std::move(payload));
    };

    fwUpdateInProgress = true;

    fwUpdateMatcher = std::make_unique<sdbusplus::bus::match_t>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',path='/xyz/openbmc_project/software'",
        callback);

    fwUpdateErrorMatcher = std::make_unique<sdbusplus::bus::match_t>(
        *crow::connections::systemBus,
        "interface='org.freedesktop.DBus.ObjectManager',type='signal',"
        "member='InterfacesAdded',"
        "path='/xyz/openbmc_project/logging'",
        std::bind_front(afterUpdateErrorMatcher, asyncResp, url));
}

struct TftpUrl
{
    std::string fwFile;
    std::string tftpServer;
};

inline std::optional<TftpUrl> parseTftpUrl(
    std::string imageURI, std::optional<std::string> transferProtocol,
    crow::Response& res)
{
    if (imageURI.find("://") == std::string::npos)
    {
        if (imageURI.starts_with("/"))
        {
            messages::actionParameterValueTypeError(
                res, imageURI, "ImageURI", "UpdateService.SimpleUpdate");
            return std::nullopt;
        }
        if (!transferProtocol)
        {
            messages::actionParameterValueTypeError(
                res, imageURI, "ImageURI", "UpdateService.SimpleUpdate");
            return std::nullopt;
        }
        // OpenBMC currently only supports TFTP
        if (*transferProtocol != "TFTP")
        {
            messages::actionParameterNotSupported(res, "TransferProtocol",
                                                  *transferProtocol);
            BMCWEB_LOG_ERROR("Request incorrect protocol parameter: {}",
                             *transferProtocol);
            return std::nullopt;
        }
        imageURI = "tftp://" + imageURI;
    }

    boost::system::result<boost::urls::url> url =
        boost::urls::parse_absolute_uri(imageURI);
    if (!url)
    {
        messages::actionParameterValueTypeError(res, imageURI, "ImageURI",
                                                "UpdateService.SimpleUpdate");

        return std::nullopt;
    }
    url->normalize();

    if (url->scheme() != "tftp")
    {
        messages::actionParameterNotSupported(res, "ImageURI", imageURI);
        return std::nullopt;
    }
    std::string path(url->encoded_path());
    if (path.size() < 2)
    {
        messages::actionParameterNotSupported(res, "ImageURI", imageURI);
        return std::nullopt;
    }
    path.erase(0, 1);
    std::string host(url->encoded_host_and_port());
    return TftpUrl{path, host};
}

/**
 * UpdateServiceActionsSimpleUpdate class supports handle POST method for
 * SimpleUpdate action.
 */
inline void requestRoutesUpdateServiceActionsSimpleUpdate(App& app)
{
    BMCWEB_ROUTE(
        app, "/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate/")
        .privileges(redfish::privileges::postUpdateService)
        .methods(
            boost::beast::http::verb::
                post)([&app](
                          const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp))
            {
                return;
            }

            std::optional<std::string> transferProtocol;
            std::string imageURI;

            BMCWEB_LOG_DEBUG("Enter UpdateService.SimpleUpdate doPost");

            // User can pass in both TransferProtocol and ImageURI parameters or
            // they can pass in just the ImageURI with the transfer protocol
            // embedded within it.
            // 1) TransferProtocol:TFTP ImageURI:1.1.1.1/myfile.bin
            // 2) ImageURI:tftp://1.1.1.1/myfile.bin

            if (!json_util::readJsonAction(req, asyncResp->res,
                                           "TransferProtocol", transferProtocol,
                                           "ImageURI", imageURI))
            {
                BMCWEB_LOG_DEBUG(
                    "Missing TransferProtocol or ImageURI parameter");
                return;
            }
            std::optional<TftpUrl> ret =
                parseTftpUrl(imageURI, transferProtocol, asyncResp->res);
            if (!ret)
            {
                return;
            }

            BMCWEB_LOG_DEBUG("Server: {} File: {}", ret->tftpServer,
                             ret->fwFile);

            // Setup callback for when new software detected
            // Give TFTP 10 minutes to complete
            monitorForSoftwareAvailable(
                asyncResp, req,
                "/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate",
                600);

            // TFTP can take up to 10 minutes depending on image size and
            // connection speed. Return to caller as soon as the TFTP operation
            // has been started. The callback above will ensure the activate
            // is started once the download has completed
            redfish::messages::success(asyncResp->res);

            // Call TFTP service
            crow::connections::systemBus->async_method_call(
                [](const boost::system::error_code& ec) {
                    if (ec)
                    {
                        // messages::internalError(asyncResp->res);
                        cleanUp();
                        BMCWEB_LOG_DEBUG("error_code = {}", ec);
                        BMCWEB_LOG_DEBUG("error msg = {}", ec.message());
                    }
                    else
                    {
                        BMCWEB_LOG_DEBUG("Call to DownloaViaTFTP Success");
                    }
                },
                "xyz.openbmc_project.Software.Download",
                "/xyz/openbmc_project/software",
                "xyz.openbmc_project.Common.TFTP", "DownloadViaTFTP",
                ret->fwFile, ret->tftpServer);

            BMCWEB_LOG_DEBUG("Exit UpdateService.SimpleUpdate doPost");
        });
}

inline void uploadImageFile(crow::Response& res, std::string_view body)
{
    std::filesystem::path filepath("/tmp/images/" + bmcweb::getRandomUUID());

    BMCWEB_LOG_DEBUG("Writing file to {}", filepath.string());
    std::ofstream out(filepath, std::ofstream::out | std::ofstream::binary |
                                    std::ofstream::trunc);
    // set the permission of the file to 640
    std::filesystem::perms permission =
        std::filesystem::perms::owner_read | std::filesystem::perms::group_read;
    std::filesystem::permissions(filepath, permission);
    out << body;

    if (out.bad())
    {
        messages::internalError(res);
        cleanUp();
    }
}

inline void setApplyTime(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::string& applyTime)
{
    std::string applyTimeNewVal;
    if (applyTime == "Immediate")
    {
        applyTimeNewVal =
            "xyz.openbmc_project.Software.ApplyTime.RequestedApplyTimes.Immediate";
    }
    else if (applyTime == "OnReset")
    {
        applyTimeNewVal =
            "xyz.openbmc_project.Software.ApplyTime.RequestedApplyTimes.OnReset";
    }
    else
    {
        BMCWEB_LOG_INFO(
            "ApplyTime value is not in the list of acceptable values");
        messages::propertyValueNotInList(asyncResp->res, applyTime,
                                         "ApplyTime");
        return;
    }

    setDbusProperty(asyncResp, "xyz.openbmc_project.Settings",
                    sdbusplus::message::object_path(
                        "/xyz/openbmc_project/software/apply_time"),
                    "xyz.openbmc_project.Software.ApplyTime",
                    "RequestedApplyTime", "ApplyTime", applyTimeNewVal);
}

inline void updateMultipartContext(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const crow::Request& req, const MultipartParser& parser)
{
    const std::string* uploadData = nullptr;
    std::optional<std::string> applyTime = "OnReset";
    bool targetFound = false;
    for (const FormPart& formpart : parser.mime_fields)
    {
        boost::beast::http::fields::const_iterator it =
            formpart.fields.find("Content-Disposition");
        if (it == formpart.fields.end())
        {
            BMCWEB_LOG_ERROR("Couldn't find Content-Disposition");
            return;
        }
        BMCWEB_LOG_INFO("Parsing value {}", it->value());

        // The construction parameters of param_list must start with `;`
        size_t index = it->value().find(';');
        if (index == std::string::npos)
        {
            continue;
        }

        for (const auto& param :
             boost::beast::http::param_list{it->value().substr(index)})
        {
            if (param.first != "name" || param.second.empty())
            {
                continue;
            }

            if (param.second == "UpdateParameters")
            {
                std::vector<std::string> targets;
                nlohmann::json content =
                    nlohmann::json::parse(formpart.content);
                if (!json_util::readJson(content, asyncResp->res, "Targets",
                                         targets, "@Redfish.OperationApplyTime",
                                         applyTime))
                {
                    return;
                }
                if (targets.size() != 1)
                {
                    messages::propertyValueFormatError(asyncResp->res,
                                                       "Targets", "");
                    return;
                }
                if (targets[0] != "/redfish/v1/Managers/bmc")
                {
                    messages::propertyValueNotInList(asyncResp->res,
                                                     "Targets/0", targets[0]);
                    return;
                }
                targetFound = true;
            }
            else if (param.second == "UpdateFile")
            {
                uploadData = &(formpart.content);
            }
        }
    }

    if (uploadData == nullptr)
    {
        BMCWEB_LOG_ERROR("Upload data is NULL");
        messages::propertyMissing(asyncResp->res, "UpdateFile");
        return;
    }
    if (!targetFound)
    {
        messages::propertyMissing(asyncResp->res, "targets");
        return;
    }

    setApplyTime(asyncResp, *applyTime);

    // Setup callback for when new software detected
    monitorForSoftwareAvailable(asyncResp, req, "/redfish/v1/UpdateService");

    uploadImageFile(asyncResp->res, *uploadData);
}

inline void handleUpdateServicePost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& url)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    std::string_view contentType = req.getHeaderValue("Content-Type");

    BMCWEB_LOG_DEBUG("doPost: contentType={}", contentType);

    // Make sure that content type is application/octet-stream or
    // multipart/form-data
    if (bmcweb::asciiIEquals(contentType, "application/octet-stream"))
    {
        // Setup callback for when new software detected
        monitorForSoftwareAvailable(asyncResp, req, url);

        uploadImageFile(asyncResp->res, req.body());
    }
    else if (contentType.starts_with("multipart/form-data"))
    {
        MultipartParser parser;

        ParserError ec = parser.parse(req);
        if (ec != ParserError::PARSER_SUCCESS)
        {
            // handle error
            BMCWEB_LOG_ERROR("MIME parse failed, ec : {}",
                             static_cast<int>(ec));
            messages::internalError(asyncResp->res);
            return;
        }

        updateMultipartContext(asyncResp, req, parser);
    }
    else
    {
        BMCWEB_LOG_DEBUG("Bad content type specified:{}", contentType);
        asyncResp->res.result(boost::beast::http::status::bad_request);
    }
}

/**
 * UpdateServiceActionsOemConcurrentUpdate class supports handle POST method for
 * concurrent update action.
 */
inline void requestRoutesUpdateServiceActionsOemConcurrentUpdate(App& app)
{
    BMCWEB_ROUTE(
        app,
        "/redfish/v1/UpdateService/Actions/Oem/OemUpdateService.ConcurrentUpdate/")
        .privileges(redfish::privileges::postUpdateService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            [&app](App&, const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                handleUpdateServicePost(
                    app, req, asyncResp,
                    "/redfish/v1/UpdateService/Actions/Oem/OemUpdateService.ConcurrentUpdate");
            },
            std::ref(app)));
}

inline void requestRoutesUpdateService(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/UpdateService/")
        .privileges(redfish::privileges::getUpdateService)
        .methods(
            boost::beast::http::verb::
                get)([&app](
                         const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp))
            {
                return;
            }
            asyncResp->res.jsonValue["@odata.type"] =
                "#UpdateService.v1_11_1.UpdateService";
            asyncResp->res.jsonValue["@odata.id"] = "/redfish/v1/UpdateService";
            asyncResp->res.jsonValue["Id"] = "UpdateService";
            asyncResp->res.jsonValue["Description"] =
                "Service for Software Update";
            asyncResp->res.jsonValue["Name"] = "Update Service";

            asyncResp->res.jsonValue["HttpPushUri"] =
                "/redfish/v1/UpdateService/update";
            asyncResp->res.jsonValue["MultipartHttpPushUri"] =
                "/redfish/v1/UpdateService/update";

            // UpdateService cannot be disabled
            asyncResp->res.jsonValue["ServiceEnabled"] = true;
            asyncResp->res.jsonValue["FirmwareInventory"]["@odata.id"] =
                "/redfish/v1/UpdateService/FirmwareInventory";
            // Get the MaxImageSizeBytes
            asyncResp->res.jsonValue["MaxImageSizeBytes"] =
                BMCWEB_HTTP_BODY_LIMIT * 1024 * 1024;
            nlohmann::json& updateSvcConUpdate =
                asyncResp->res
                    .jsonValue["Actions"]["Oem"]["#OemUpdateService.v1_0_0."
                                                 "ConcurrentUpdate"];
            updateSvcConUpdate["target"] =
                "/redfish/v1/UpdateService/Actions/Oem/OemUpdateService.ConcurrentUpdate";

            if constexpr (BMCWEB_INSECURE_TFTP_UPDATE)
            {
                // Update Actions object.
                nlohmann::json& updateSvcSimpleUpdate =
                    asyncResp->res
                        .jsonValue["Actions"]["#UpdateService.SimpleUpdate"];
                updateSvcSimpleUpdate["target"] =
                    "/redfish/v1/UpdateService/Actions/UpdateService.SimpleUpdate";
                updateSvcSimpleUpdate
                    ["TransferProtocol@Redfish.AllowableValues"] = {"TFTP"};
            }
            // Get the current ApplyTime value
            sdbusplus::asio::getProperty<std::string>(
                *crow::connections::systemBus, "xyz.openbmc_project.Settings",
                "/xyz/openbmc_project/software/apply_time",
                "xyz.openbmc_project.Software.ApplyTime", "RequestedApplyTime",
                [asyncResp](const boost::system::error_code& ec,
                            const std::string& applyTime) {
                    if (ec)
                    {
                        BMCWEB_LOG_DEBUG("DBUS response error {}", ec);
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // Store the ApplyTime Value
                    if (applyTime == "xyz.openbmc_project.Software.ApplyTime."
                                     "RequestedApplyTimes.Immediate")
                    {
                        asyncResp->res
                            .jsonValue["HttpPushUriOptions"]
                                      ["HttpPushUriApplyTime"]["ApplyTime"] =
                            update_service::ApplyTime::Immediate;
                    }
                    else if (applyTime ==
                             "xyz.openbmc_project.Software.ApplyTime."
                             "RequestedApplyTimes.OnReset")
                    {
                        asyncResp->res
                            .jsonValue["HttpPushUriOptions"]
                                      ["HttpPushUriApplyTime"]["ApplyTime"] =
                            update_service::ApplyTime::OnReset;
                    }
                });
        });
    BMCWEB_ROUTE(app, "/redfish/v1/UpdateService/")
        .privileges(redfish::privileges::patchUpdateService)
        .methods(boost::beast::http::verb::patch)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                BMCWEB_LOG_DEBUG("doPatch...");

                std::optional<nlohmann::json> pushUriOptions;
                if (!json_util::readJsonPatch(req, asyncResp->res,
                                              "HttpPushUriOptions",
                                              pushUriOptions))
                {
                    return;
                }

                if (pushUriOptions)
                {
                    std::optional<nlohmann::json> pushUriApplyTime;
                    if (!json_util::readJson(*pushUriOptions, asyncResp->res,
                                             "HttpPushUriApplyTime",
                                             pushUriApplyTime))
                    {
                        return;
                    }

                    if (pushUriApplyTime)
                    {
                        std::optional<std::string> applyTime;
                        if (!json_util::readJson(*pushUriApplyTime,
                                                 asyncResp->res, "ApplyTime",
                                                 applyTime))
                        {
                            return;
                        }

                        if (applyTime)
                        {
                            setApplyTime(asyncResp, *applyTime);
                        }
                    }
                }
            });

    BMCWEB_ROUTE(app, "/redfish/v1/UpdateService/update/")
        .privileges(redfish::privileges::postUpdateService)
        .methods(boost::beast::http::verb::post)(std::bind_front(
            [&app](App&, const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                handleUpdateServicePost(app, req, asyncResp,
                                        "/redfish/v1/UpdateService");
            },
            std::ref(app)));
}

inline void requestRoutesSoftwareInventoryCollection(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/UpdateService/FirmwareInventory/")
        .privileges(redfish::privileges::getSoftwareInventoryCollection)
        .methods(boost::beast::http::verb::get)(
            [&app](const crow::Request& req,
                   const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (!redfish::setUpRedfishRoute(app, req, asyncResp))
                {
                    return;
                }
                asyncResp->res.jsonValue["@odata.type"] =
                    "#SoftwareInventoryCollection.SoftwareInventoryCollection";
                asyncResp->res.jsonValue["@odata.id"] =
                    "/redfish/v1/UpdateService/FirmwareInventory";
                asyncResp->res.jsonValue["Name"] =
                    "Software Inventory Collection";
                const std::array<const std::string_view, 1> iface = {
                    "xyz.openbmc_project.Software.Version"};

                redfish::collection_util::getCollectionMembers(
                    asyncResp,
                    boost::urls::url(
                        "/redfish/v1/UpdateService/FirmwareInventory"),
                    iface, "/xyz/openbmc_project/software");
            });
}
/* Fill related item links (i.e. bmc, bios) in for inventory */
inline static void
    getRelatedItems(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                    const std::string& purpose)
{
    if (purpose == sw_util::bmcPurpose)
    {
        nlohmann::json& relatedItem = asyncResp->res.jsonValue["RelatedItem"];
        nlohmann::json::object_t item;
        item["@odata.id"] = boost::urls::format(
            "/redfish/v1/Managers/{}", BMCWEB_REDFISH_MANAGER_URI_NAME);
        relatedItem.emplace_back(std::move(item));
        asyncResp->res.jsonValue["RelatedItem@odata.count"] =
            relatedItem.size();
    }
    else if (purpose == sw_util::biosPurpose)
    {
        nlohmann::json& relatedItem = asyncResp->res.jsonValue["RelatedItem"];
        nlohmann::json::object_t item;
        item["@odata.id"] = std::format("/redfish/v1/Systems/{}/Bios",
                                        BMCWEB_REDFISH_SYSTEM_URI_NAME);
        relatedItem.emplace_back(std::move(item));
        asyncResp->res.jsonValue["RelatedItem@odata.count"] =
            relatedItem.size();
    }
    else
    {
        BMCWEB_LOG_DEBUG("Unknown software purpose {}", purpose);
    }
}

inline void
    getSoftwareVersion(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& service, const std::string& path,
                       const std::string& swId)
{
    sdbusplus::asio::getAllProperties(
        *crow::connections::systemBus, service, path,
        "xyz.openbmc_project.Software.Version",
        [asyncResp,
         swId](const boost::system::error_code& ec,
               const dbus::utility::DBusPropertiesMap& propertiesList) {
            if (ec)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            const std::string* swInvPurpose = nullptr;
            const std::string* version = nullptr;

            const bool success = sdbusplus::unpackPropertiesNoThrow(
                dbus_utils::UnpackErrorPrinter(), propertiesList, "Purpose",
                swInvPurpose, "Version", version);

            if (!success)
            {
                messages::internalError(asyncResp->res);
                return;
            }

            if (swInvPurpose == nullptr)
            {
                BMCWEB_LOG_DEBUG("Can't find property \"Purpose\"!");
                messages::internalError(asyncResp->res);
                return;
            }

            BMCWEB_LOG_DEBUG("swInvPurpose = {}", *swInvPurpose);

            if (version == nullptr)
            {
                BMCWEB_LOG_DEBUG("Can't find property \"Version\"!");

                messages::internalError(asyncResp->res);

                return;
            }
            asyncResp->res.jsonValue["Version"] = *version;
            asyncResp->res.jsonValue["Id"] = swId;

            // swInvPurpose is of format:
            // xyz.openbmc_project.Software.Version.VersionPurpose.ABC
            // Translate this to "ABC image"
            size_t endDesc = swInvPurpose->rfind('.');
            if (endDesc == std::string::npos)
            {
                messages::internalError(asyncResp->res);
                return;
            }
            endDesc++;
            if (endDesc >= swInvPurpose->size())
            {
                messages::internalError(asyncResp->res);
                return;
            }

            std::string formatDesc = swInvPurpose->substr(endDesc);
            asyncResp->res.jsonValue["Description"] = formatDesc + " image";
            getRelatedItems(asyncResp, *swInvPurpose);
        });
}

inline void requestRoutesSoftwareInventory(App& app)
{
    BMCWEB_ROUTE(app, "/redfish/v1/UpdateService/FirmwareInventory/<str>/")
        .privileges(redfish::privileges::getSoftwareInventory)
        .methods(
            boost::beast::http::verb::
                get)([&app](const crow::Request& req,
                            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const std::string& param) {
            if (!redfish::setUpRedfishRoute(app, req, asyncResp))
            {
                return;
            }
            std::shared_ptr<std::string> swId =
                std::make_shared<std::string>(param);

            asyncResp->res.jsonValue["@odata.id"] = boost::urls::format(
                "/redfish/v1/UpdateService/FirmwareInventory/{}", *swId);

            constexpr std::array<std::string_view, 1> interfaces = {
                "xyz.openbmc_project.Software.Version"};
            dbus::utility::getSubTree(
                "/", 0, interfaces,
                [asyncResp,
                 swId](const boost::system::error_code& ec,
                       const dbus::utility::MapperGetSubTreeResponse& subtree) {
                    BMCWEB_LOG_DEBUG("doGet callback...");
                    if (ec)
                    {
                        messages::internalError(asyncResp->res);
                        return;
                    }

                    // Ensure we find our input swId, otherwise return an error
                    bool found = false;
                    for (const std::pair<
                             std::string,
                             std::vector<std::pair<
                                 std::string, std::vector<std::string>>>>& obj :
                         subtree)
                    {
                        if (!obj.first.ends_with(*swId))
                        {
                            continue;
                        }

                        if (obj.second.empty())
                        {
                            continue;
                        }

                        found = true;
                        sw_util::getSwStatus(asyncResp, swId,
                                             obj.second[0].first);
                        sw_util::getSwMinimumVersion(asyncResp, swId,
                                                     obj.second[0].first);

                        getSoftwareVersion(asyncResp, obj.second[0].first,
                                           obj.first, *swId);
                    }
                    if (!found)
                    {
                        BMCWEB_LOG_WARNING("Input swID {} not found!", *swId);
                        messages::resourceMissingAtURI(
                            asyncResp->res,
                            boost::urls::format(
                                "/redfish/v1/UpdateService/FirmwareInventory/{}",
                                *swId));
                        return;
                    }
                    asyncResp->res.jsonValue["@odata.type"] =
                        "#SoftwareInventory.v1_1_0.SoftwareInventory";
                    asyncResp->res.jsonValue["Name"] = "Software Inventory";
                    asyncResp->res.jsonValue["Status"]["HealthRollup"] =
                        resource::Health::OK;

                    asyncResp->res.jsonValue["Updateable"] = false;
                    sw_util::getSwUpdatableStatus(asyncResp, swId);
                });
        });
}

} // namespace redfish
