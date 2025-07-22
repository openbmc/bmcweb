// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
// SPDX-FileCopyrightText: Copyright 2018 Intel Corporation
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "dbus_utility.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "logging.hpp"
#include "multipart_parser.hpp"
#include "task.hpp"

#include <sys/mman.h>
#include <unistd.h>

#include <boost/asio/error.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/http/fields.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <boost/system/error_code.hpp>
#include <boost/system/result.hpp>
#include <boost/url/format.hpp>
#include <boost/url/parse.hpp>
#include <boost/url/url.hpp>
#include <boost/url/url_view.hpp>
#include <boost/url/url_view_base.hpp>
#include <sdbusplus/asio/property.hpp>
#include <sdbusplus/bus/match.hpp>
#include <sdbusplus/message.hpp>
#include <sdbusplus/message/native_types.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <cstdio>
#include <format>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace redfish
{

struct MemoryFileDescriptor
{
    int fd = -1;

    explicit MemoryFileDescriptor(const std::string& filename) :
        fd(memfd_create(filename.c_str(), 0))
    {}

    MemoryFileDescriptor(const MemoryFileDescriptor&) = default;
    MemoryFileDescriptor(MemoryFileDescriptor&& other) noexcept : fd(other.fd)
    {
        other.fd = -1;
    }
    MemoryFileDescriptor& operator=(const MemoryFileDescriptor&) = delete;
    MemoryFileDescriptor& operator=(MemoryFileDescriptor&&) = default;

    ~MemoryFileDescriptor()
    {
        if (fd != -1)
        {
            close(fd);
        }
    }

    bool rewind() const
    {
        if (lseek(fd, 0, SEEK_SET) == -1)
        {
            BMCWEB_LOG_ERROR("Failed to seek to beginning of image memfd");
            return false;
        }
        return true;
    }
};

void cleanUp();

void activateImage(const std::string& objPath, const std::string& service);

bool handleCreateTask(const boost::system::error_code& ec2,
                      sdbusplus::message_t& msg,
                      const std::shared_ptr<task::TaskData>& taskData);

void createTask(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                task::Payload&& payload,
                const sdbusplus::message::object_path& objPath);

// Note that asyncResp can be either a valid pointer or nullptr. If nullptr
// then no asyncResp updates will occur
void softwareInterfaceAdded(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            sdbusplus::message_t& m, task::Payload&& payload);

void afterAvailbleTimerAsyncWait(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const boost::system::error_code& ec);

void handleUpdateErrorType(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& url, const std::string& type);

void afterUpdateErrorMatcher(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp, const std::string& url,
    sdbusplus::message_t& m);

// Note that asyncResp can be either a valid pointer or nullptr. If nullptr
// then no asyncResp updates will occur
void monitorForSoftwareAvailable(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const crow::Request& req, const std::string& url,
    int timeoutTimeSeconds = 50);

std::optional<boost::urls::url> parseSimpleUpdateUrl(
    std::string imageURI, std::optional<std::string> transferProtocol,
    crow::Response& res);

void doHttpsUpdate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                   const boost::urls::url_view_base& url);

void handleUpdateServiceSimpleUpdateAction(
    crow::App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void uploadImageFile(crow::Response& res, std::string_view body);

// Convert the Request Apply Time to the D-Bus value
bool convertApplyTime(crow::Response& res, const std::string& applyTime,
                      std::string& applyTimeNewVal);

void setApplyTime(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const std::string& applyTime);

struct MultiPartUpdateParameters
{
    std::optional<std::string> applyTime;
    std::string uploadData;
    std::vector<std::string> targets;
};

std::optional<std::string> processUrl(
    boost::system::result<boost::urls::url_view>& url);

std::optional<MultiPartUpdateParameters> extractMultipartUpdateParameters(
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    MultipartParser parser);

void handleStartUpdate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       task::Payload payload, const std::string& objectPath,
                       const boost::system::error_code& ec,
                       const sdbusplus::message::object_path& retPath);

void startUpdate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                 task::Payload payload, const MemoryFileDescriptor& memfd,
                 const std::string& applyTime, const std::string& objectPath,
                 const std::string& serviceName);

void getSwInfo(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               task::Payload payload, const MemoryFileDescriptor& memfd,
               const std::string& applyTime, const std::string& target,
               const boost::system::error_code& ec,
               const dbus::utility::MapperGetSubTreeResponse& subtree);

void handleBMCUpdate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     task::Payload payload, const MemoryFileDescriptor& memfd,
                     const std::string& applyTime,
                     const boost::system::error_code& ec,
                     const dbus::utility::MapperEndPoints& functionalSoftware);

void processUpdateRequest(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          task::Payload&& payload, std::string_view body,
                          const std::string& applyTime,
                          std::vector<std::string>& targets);

void updateMultipartContext(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                            const crow::Request& req, MultipartParser&& parser);

void doHTTPUpdate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                  const crow::Request& req);

void handleUpdateServicePost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void handleUpdateServiceMultipartUpdatePost(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void handleUpdateServiceGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

void handleUpdateServiceFirmwareInventoryCollectionGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp);

/* Fill related item links (i.e. bmc, bios) in for inventory */
void getRelatedItems(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& purpose);

void getSoftwareVersion(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                        const std::string& service, const std::string& path,
                        const std::string& swId);

void handleUpdateServiceFirmwareInventoryGet(
    App& app, const crow::Request& req,
    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
    const std::string& param);

void requestRoutesUpdateService(App& app);

} // namespace redfish
