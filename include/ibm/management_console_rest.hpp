// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
#pragma once

#include "app.hpp"
#include "async_resp.hpp"
#include "error_messages.hpp"
#include "event_service_manager.hpp"
#include "ibm/utils.hpp"
#include "resource_messages.hpp"
#include "str_utility.hpp"
#include "utils/json_utils.hpp"

#include <boost/container/flat_set.hpp>
#include <nlohmann/json.hpp>
#include <sdbusplus/message/types.hpp>

#include <filesystem>
#include <fstream>

namespace crow
{
namespace ibm_mc
{
constexpr const char* methodNotAllowedMsg = "Method Not Allowed";
constexpr const char* resourceNotFoundMsg = "Resource Not Found";
constexpr const char* contentNotAcceptableMsg = "Content Not Acceptable";
constexpr const char* internalServerError = "Internal Server Error";

constexpr size_t maxSaveareaDirSize =
    25000000; // Allow save area dir size to be max 25MB
constexpr size_t minSaveareaFileSize =
    100;      // Allow save area file size of minimum 100B
constexpr size_t maxSaveareaFileSize =
    500000;   // Allow save area file size upto 500KB
constexpr size_t maxBroadcastMsgSize =
    1000;     // Allow Broadcast message size upto 1KB

inline void handleFilePut(const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fileID)
{
    std::error_code ec;
    // Check the content-type of the request
    boost::beast::string_view contentType = req.getHeaderValue("content-type");
    if (!bmcweb::asciiIEquals(contentType, "application/octet-stream"))
    {
        asyncResp->res.result(boost::beast::http::status::not_acceptable);
        asyncResp->res.jsonValue["Description"] = contentNotAcceptableMsg;
        return;
    }
    BMCWEB_LOG_DEBUG(
        "File upload in application/octet-stream format. Continue..");

    BMCWEB_LOG_DEBUG(
        "handleIbmPut: Request to create/update the save-area file");
    std::string_view path =
        "/var/lib/bmcweb/ibm-management-console/configfiles";
    if (!crow::ibm_utils::createDirectory(path))
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    std::ofstream file;
    std::filesystem::path loc(
        "/var/lib/bmcweb/ibm-management-console/configfiles");

    // Get the current size of the savearea directory
    std::filesystem::recursive_directory_iterator iter(loc, ec);
    if (ec)
    {
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        asyncResp->res.jsonValue["Description"] = internalServerError;
        BMCWEB_LOG_DEBUG("handleIbmPut: Failed to prepare save-area "
                         "directory iterator. ec : {}",
                         ec.message());
        return;
    }
    std::uintmax_t saveAreaDirSize = 0;
    for (const auto& it : iter)
    {
        if (!std::filesystem::is_directory(it, ec))
        {
            if (ec)
            {
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                BMCWEB_LOG_DEBUG("handleIbmPut: Failed to find save-area "
                                 "directory . ec : {}",
                                 ec.message());
                return;
            }
            std::uintmax_t fileSize = std::filesystem::file_size(it, ec);
            if (ec)
            {
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                BMCWEB_LOG_DEBUG("handleIbmPut: Failed to find save-area "
                                 "file size inside the directory . ec : {}",
                                 ec.message());
                return;
            }
            saveAreaDirSize += fileSize;
        }
    }
    BMCWEB_LOG_DEBUG("saveAreaDirSize: {}", saveAreaDirSize);

    // Get the file size getting uploaded
    const std::string& data = req.body();
    BMCWEB_LOG_DEBUG("data length: {}", data.length());

    if (data.length() < minSaveareaFileSize)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue["Description"] =
            "File size is less than minimum allowed size[100B]";
        return;
    }
    if (data.length() > maxSaveareaFileSize)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue["Description"] =
            "File size exceeds maximum allowed size[500KB]";
        return;
    }

    // Form the file path
    loc /= fileID;
    BMCWEB_LOG_DEBUG("Writing to the file: {}", loc.string());

    // Check if the same file exists in the directory
    bool fileExists = std::filesystem::exists(loc, ec);
    if (ec)
    {
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        asyncResp->res.jsonValue["Description"] = internalServerError;
        BMCWEB_LOG_DEBUG("handleIbmPut: Failed to find if file exists. ec : {}",
                         ec.message());
        return;
    }

    std::uintmax_t newSizeToWrite = 0;
    if (fileExists)
    {
        // File exists. Get the current file size
        std::uintmax_t currentFileSize = std::filesystem::file_size(loc, ec);
        if (ec)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            BMCWEB_LOG_DEBUG("handleIbmPut: Failed to find file size. ec : {}",
                             ec.message());
            return;
        }
        // Calculate the difference in the file size.
        // If the data.length is greater than the existing file size, then
        // calculate the difference. Else consider the delta size as zero -
        // because there is no increase in the total directory size.
        // We need to add the diff only if the incoming data is larger than the
        // existing filesize
        if (data.length() > currentFileSize)
        {
            newSizeToWrite = data.length() - currentFileSize;
        }
        BMCWEB_LOG_DEBUG("newSizeToWrite: {}", newSizeToWrite);
    }
    else
    {
        // This is a new file upload
        newSizeToWrite = data.length();
    }

    // Calculate the total dir size before writing the new file
    BMCWEB_LOG_DEBUG("total new size: {}", saveAreaDirSize + newSizeToWrite);

    if ((saveAreaDirSize + newSizeToWrite) > maxSaveareaDirSize)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue["Description"] =
            "File size does not fit in the savearea "
            "directory maximum allowed size[25MB]";
        return;
    }

    file.open(loc, std::ofstream::out);

    // set the permission of the file to 600
    std::filesystem::perms permission = std::filesystem::perms::owner_write |
                                        std::filesystem::perms::owner_read;
    std::filesystem::permissions(loc, permission);

    if (file.fail())
    {
        BMCWEB_LOG_DEBUG("Error while opening the file for writing");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        asyncResp->res.jsonValue["Description"] =
            "Error while creating the file";
        return;
    }
    file << data;

    // Push an event
    if (fileExists)
    {
        BMCWEB_LOG_DEBUG("config file is updated");
        asyncResp->res.jsonValue["Description"] = "File Updated";
    }
    else
    {
        BMCWEB_LOG_DEBUG("config file is created");
        asyncResp->res.jsonValue["Description"] = "File Created";
    }
}

inline void
    handleConfigFileList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::vector<std::string> pathObjList;
    std::filesystem::path loc(
        "/var/lib/bmcweb/ibm-management-console/configfiles");
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        for (const auto& file : std::filesystem::directory_iterator(loc))
        {
            const std::filesystem::path& pathObj = file.path();
            if (std::filesystem::is_regular_file(pathObj))
            {
                pathObjList.emplace_back(
                    "/ibm/v1/Host/ConfigFiles/" + pathObj.filename().string());
            }
        }
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#IBMConfigFile.v1_0_0.IBMConfigFile";
    asyncResp->res.jsonValue["@odata.id"] = "/ibm/v1/Host/ConfigFiles/";
    asyncResp->res.jsonValue["Id"] = "ConfigFiles";
    asyncResp->res.jsonValue["Name"] = "ConfigFiles";

    asyncResp->res.jsonValue["Members"] = std::move(pathObjList);
    asyncResp->res.jsonValue["Actions"]["#IBMConfigFiles.DeleteAll"]["target"] =
        "/ibm/v1/Host/ConfigFiles/Actions/IBMConfigFiles.DeleteAll";
}

inline void
    deleteConfigFiles(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::error_code ec;
    std::filesystem::path loc(
        "/var/lib/bmcweb/ibm-management-console/configfiles");
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        std::filesystem::remove_all(loc, ec);
        if (ec)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            BMCWEB_LOG_DEBUG("deleteConfigFiles: Failed to delete the "
                             "config files directory. ec : {}",
                             ec.message());
        }
    }
}

inline void handleFileGet(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fileID)
{
    BMCWEB_LOG_DEBUG("HandleGet on SaveArea files on path: {}", fileID);
    std::filesystem::path loc(
        "/var/lib/bmcweb/ibm-management-console/configfiles/" + fileID);
    if (!std::filesystem::exists(loc) || !std::filesystem::is_regular_file(loc))
    {
        BMCWEB_LOG_WARNING("{} Not found", loc.string());
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    std::ifstream readfile(loc.string());
    if (!readfile)
    {
        BMCWEB_LOG_WARNING("{} Not found", loc.string());
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    std::string contentDispositionParam =
        "attachment; filename=\"" + fileID + "\"";
    asyncResp->res.addHeader(boost::beast::http::field::content_disposition,
                             contentDispositionParam);
    std::string fileData;
    fileData = {std::istreambuf_iterator<char>(readfile),
                std::istreambuf_iterator<char>()};
    asyncResp->res.jsonValue["Data"] = fileData;
}

inline void
    handleFileDelete(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& fileID)
{
    std::string filePath(
        "/var/lib/bmcweb/ibm-management-console/configfiles/" + fileID);
    BMCWEB_LOG_DEBUG("Removing the file : {}", filePath);
    std::ifstream fileOpen(filePath.c_str());
    if (static_cast<bool>(fileOpen))
    {
        if (remove(filePath.c_str()) == 0)
        {
            BMCWEB_LOG_DEBUG("File removed!");
            asyncResp->res.jsonValue["Description"] = "File Deleted";
        }
        else
        {
            BMCWEB_LOG_ERROR("File not removed!");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
        }
    }
    else
    {
        BMCWEB_LOG_WARNING("File not found!");
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.jsonValue["Description"] = resourceNotFoundMsg;
    }
}

inline void
    handleBroadcastService(const crow::Request& req,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::string broadcastMsg;

    if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "Message",
                                           broadcastMsg))
    {
        BMCWEB_LOG_DEBUG("Not a Valid JSON");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    if (broadcastMsg.size() > maxBroadcastMsgSize)
    {
        BMCWEB_LOG_ERROR("Message size exceeds maximum allowed size[1KB]");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
}

inline void handleFileUrl(const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fileID)
{
    if (req.method() == boost::beast::http::verb::put)
    {
        handleFilePut(req, asyncResp, fileID);
        return;
    }
    if (req.method() == boost::beast::http::verb::get)
    {
        handleFileGet(asyncResp, fileID);
        return;
    }
    if (req.method() == boost::beast::http::verb::delete_)
    {
        handleFileDelete(asyncResp, fileID);
        return;
    }
}

inline bool isValidConfigFileName(const std::string& fileName,
                                  crow::Response& res)
{
    if (fileName.empty())
    {
        BMCWEB_LOG_ERROR("Empty filename");
        res.jsonValue["Description"] = "Empty file path in the url";
        return false;
    }

    // ConfigFile name is allowed to take upper and lowercase letters,
    // numbers and hyphen
    std::size_t found = fileName.find_first_not_of(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-");
    if (found != std::string::npos)
    {
        BMCWEB_LOG_ERROR("Unsupported character in filename: {}", fileName);
        res.jsonValue["Description"] = "Unsupported character in filename";
        return false;
    }

    // Check the filename length
    if (fileName.length() > 20)
    {
        BMCWEB_LOG_ERROR("Name must be maximum 20 characters. "
                         "Input filename length is: {}",
                         fileName.length());
        res.jsonValue["Description"] = "Filename must be maximum 20 characters";
        return false;
    }

    return true;
}

inline void requestRoutes(App& app)
{
    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                asyncResp->res.jsonValue["@odata.type"] =
                    "#ibmServiceRoot.v1_0_0.ibmServiceRoot";
                asyncResp->res.jsonValue["@odata.id"] = "/ibm/v1/";
                asyncResp->res.jsonValue["Id"] = "IBM Rest RootService";
                asyncResp->res.jsonValue["Name"] = "IBM Service Root";
                asyncResp->res.jsonValue["ConfigFiles"]["@odata.id"] =
                    "/ibm/v1/Host/ConfigFiles";
                asyncResp->res.jsonValue["BroadcastService"]["@odata.id"] =
                    "/ibm/v1/HMC/BroadcastService";
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                handleConfigFileList(asyncResp);
            });

    BMCWEB_ROUTE(app,
                 "/ibm/v1/Host/ConfigFiles/Actions/IBMConfigFiles.DeleteAll")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                deleteConfigFiles(asyncResp);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles/<str>")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::put, boost::beast::http::verb::get,
                 boost::beast::http::verb::delete_)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& fileName) {
                BMCWEB_LOG_DEBUG("ConfigFile : {}", fileName);
                // Validate the incoming fileName
                if (!isValidConfigFileName(fileName, asyncResp->res))
                {
                    asyncResp->res.result(
                        boost::beast::http::status::bad_request);
                    return;
                }
                handleFileUrl(req, asyncResp, fileName);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/BroadcastService")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                handleBroadcastService(req, asyncResp);
            });
}

} // namespace ibm_mc
} // namespace crow
