#pragma once

#include "multipart_parser.hpp"

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/beast/http/rfc7230.hpp>
#include <boost/container/flat_map.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_utility.hpp>
#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <ibm/locks.hpp>
#include <nlohmann/json.hpp>
#include <resource_messages.hpp>
#include <sdbusplus/message/types.hpp>
#include <utils/json_utils.hpp>

#include <filesystem>
#include <fstream>

using SType = std::string;
using SegmentFlags = std::vector<std::pair<std::string, uint32_t>>;
using LockRequest = std::tuple<SType, SType, SType, uint64_t, SegmentFlags>;
using LockRequests = std::vector<LockRequest>;
using Rc = std::pair<bool, std::variant<uint32_t, LockRequest>>;
using RcGetLockList =
    std::variant<std::string, std::vector<std::pair<uint32_t, LockRequests>>>;
using ListOfSessionIds = std::vector<std::string>;
namespace crow
{
namespace ibm_mc
{
constexpr const char* methodNotAllowedMsg = "Method Not Allowed";
constexpr const char* resourceNotFoundMsg = "Resource Not Found";
constexpr const char* contentNotAcceptableMsg = "Content Not Acceptable";
constexpr const char* internalServerError = "Internal Server Error";
constexpr uint32_t maxCSRLength = 4096;
static const std::filesystem::path rootCertPath =
    "/var/lib/ibm/bmcweb/RootCert";
constexpr const char* internalFileSystemError = "Internal FileSystem Error";
constexpr const char* badRequestMsg = "Bad Request";
constexpr const char* propertyMissing = "Required Property Missing";
constexpr const char* configFilePath =
    "/var/lib/bmcweb/ibm-management-console/configfiles";

constexpr size_t maxSaveareaDirSize =
    25000000; // Allow save area dir size to be max 25MB
constexpr size_t minSaveareaFileSize =
    100;      // Allow save area file size of minimum 100B
constexpr size_t maxSaveareaFileSize =
    25000000; // Allow save area file size upto 25MB
constexpr size_t maxBroadcastMsgSize =
    1000;     // Allow Broadcast message size upto 1KB

// NOLINTBEGIN(cppcoreguidelines-avoid-non-const-global-variables)
static boost::container::flat_map<std::string,
                                  std::unique_ptr<sdbusplus::bus::match::match>>
    ackMatches;
// NOLINTEND(cppcoreguidelines-avoid-non-const-global-variables)

inline bool isValidConfigFileName(const std::string& fileName,
                                  nlohmann::json& resp)
{
    if (fileName.empty())
    {
        BMCWEB_LOG_ERROR("Empty filename");
        resp["Description"] = "Empty file path in the url";
        return false;
    }

    // ConfigFile name is allowed to take upper and lowercase letters,
    // numbers and hyphen
    std::size_t found = fileName.find_first_not_of(
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-");
    if (found != std::string::npos)
    {
        BMCWEB_LOG_ERROR("Unsupported character in filename: {}", fileName);
        resp["Description"] = "Unsupported character in filename";
        return false;
    }

    // Check the filename length
    if (fileName.length() > 20)
    {
        BMCWEB_LOG_ERROR("Name must be maximum 20 characters. "
                         "Input filename length is: {}",
                         fileName.length());
        resp["Description"] = "Filename must be maximum 20 characters";
        return false;
    }

    return true;
}

inline bool saveConfigFile(const std::string& data, const std::string& fileID,
                           const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::error_code ec;
    // Get the file size getting uploaded
    BMCWEB_LOG_DEBUG("data length: {}", data.length());
    BMCWEB_LOG_DEBUG("fileID: {} ", fileID);

    if (data.length() < minSaveareaFileSize)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue["Description"] =
            "File size is less than minimum allowed size[100B]";
        return false;
    }
    if (data.length() > maxSaveareaFileSize)
    {
        asyncResp->res.result(boost::beast::http::status::bad_request);
        asyncResp->res.jsonValue["Description"] =
            "File size exceeds maximum allowed size[25MB]";
        return false;
    }
    std::ofstream file;
    std::filesystem::path loc(configFilePath);

    // Get the current size of the savearea directory
    std::filesystem::recursive_directory_iterator iter(loc, ec);
    if (ec)
    {
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        asyncResp->res.jsonValue["Description"] = internalFileSystemError;
        BMCWEB_LOG_DEBUG("saveConfigFile: Failed to prepare save-area "
                         "directory iterator. ec : {}",
                         ec.message());
        return false;
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
                asyncResp->res.jsonValue["Description"] =
                    internalFileSystemError;
                BMCWEB_LOG_DEBUG("saveConfigFile: Failed to find save-area "
                                 "directory . ec : {}",
                                 ec.message());
                return false;
            }
            std::uintmax_t fileSize = std::filesystem::file_size(it, ec);
            if (ec)
            {
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] =
                    internalFileSystemError;
                BMCWEB_LOG_DEBUG("saveConfigFile: Failed to find save-area "
                                 "file size inside the directory . ec : {}",
                                 ec.message());
                return false;
            }
            saveAreaDirSize += fileSize;
        }
    }
    BMCWEB_LOG_DEBUG("saveAreaDirSize: {}", saveAreaDirSize);

    // Form the file path
    loc /= fileID;
    BMCWEB_LOG_DEBUG("Writing to the file: {}", loc.string());

    // Check if the same file exists in the directory
    bool fileExists = std::filesystem::exists(loc, ec);
    if (ec)
    {
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        asyncResp->res.jsonValue["Description"] = internalFileSystemError;
        BMCWEB_LOG_DEBUG(
            "saveConfigFile: Failed to find if file exists. ec : {}",
            ec.message());
        return false;
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
            asyncResp->res.jsonValue["Description"] = internalFileSystemError;
            BMCWEB_LOG_DEBUG(
                "saveConfigFile: Failed to find file size. ec : {}",
                ec.message());
            return false;
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
        return false;
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
        return false;
    }
    file << data;
    std::string origin = "/ibm/v1/Host/ConfigFiles/" + fileID;
    // Push an event
    if (fileExists)
    {
        BMCWEB_LOG_DEBUG("config file is updated");
        asyncResp->res.jsonValue["Description"] = "File Updated";

        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "IBMConfigFile");
    }
    else
    {
        BMCWEB_LOG_DEBUG("config file is created");
        asyncResp->res.jsonValue["Description"] = "File Created";

        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceCreated(), origin, "IBMConfigFile");
    }
    return true;
}

inline void
    handleFileUpload(const crow::Request& req,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& fileID = "")
{
    // Check the content-type of the request
    boost::beast::string_view contentType = req.getHeaderValue("content-type");
    BMCWEB_LOG_DEBUG("Content Type: {}", contentType);
    if (((req.method() == boost::beast::http::verb::put) &&
         (!boost::iequals(contentType, "application/octet-stream"))) ||
        ((req.method() == boost::beast::http::verb::post) &&
         (!boost::istarts_with(contentType, "multipart/form-data"))))
    {
        asyncResp->res.result(boost::beast::http::status::not_acceptable);
        asyncResp->res.jsonValue["Description"] = contentNotAcceptableMsg;
        return;
    }

    BMCWEB_LOG_DEBUG(
        "handleFileUpload: Request to create/update the save-area file");
    std::string_view path = configFilePath;
    if (!crow::ibm_utils::createDirectory(path))
    {
        asyncResp->res.result(boost::beast::http::status::not_found);
        asyncResp->res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    bool uploadStatus = true;
    // Logic to parse the data if its multipart form
    if (boost::istarts_with(contentType, "multipart/form-data"))
    {
        BMCWEB_LOG_DEBUG("This is a multipart upload");
        MultipartParser parser;
        ParserError ec = parser.parse(req);
        if (ec != ParserError::PARSER_SUCCESS)
        {
            // handle error
            BMCWEB_LOG_ERROR("MIME parse failed, ec : {}",
                             static_cast<int>(ec));
            asyncResp->res.result(boost::beast::http::status::bad_request);
            asyncResp->res.jsonValue["Description"] = badRequestMsg;
            return;
        }
        const std::string* uploadData = nullptr;
        std::string fileName;
        for (const FormPart& formpart : parser.mime_fields)
        {
            boost::beast::http::fields::const_iterator it =
                formpart.fields.find("Content-Disposition");
            if (it == formpart.fields.end())
            {
                BMCWEB_LOG_ERROR("Couldn't find Content-Disposition");
                asyncResp->res.result(boost::beast::http::status::bad_request);
                asyncResp->res.jsonValue["Description"] = badRequestMsg;
                return;
            }
            BMCWEB_LOG_DEBUG("Parsing value {}", it->value());
            // The construction parameters of param_list must start with
            // `;`
            size_t index = it->value().find(';');
            if (index == std::string::npos)
            {
                BMCWEB_LOG_ERROR("Parsing value failed {}", it->value());
                continue;
            }
            for (const auto& param :
                 boost::beast::http::param_list{it->value().substr(index)})
            {
                if (param.first == "name")
                {
                    fileName = param.second;
                    BMCWEB_LOG_DEBUG("file name : {}", fileName);

                    // Validate the incoming fileName
                    if (!isValidConfigFileName(fileName,
                                               asyncResp->res.jsonValue))
                    {
                        asyncResp->res.result(
                            boost::beast::http::status::bad_request);
                        return;
                    }
                }
                uploadData = &(formpart.content);
            }
            if ((uploadData == nullptr) || (fileName.empty()))
            {
                BMCWEB_LOG_ERROR("Upload data or filename is NULL");
                asyncResp->res.result(boost::beast::http::status::bad_request);
                asyncResp->res.jsonValue["Description"] = propertyMissing;
                return;
            }
            uploadStatus = saveConfigFile(*uploadData, fileName, asyncResp);
            if (!uploadStatus)
            {
                BMCWEB_LOG_INFO("ConfigFile upload failed!! FileName: {}",
                                fileName);
                return;
            }
            BMCWEB_LOG_INFO("ConfigFile upload complete!! Filename: {}",
                            fileName);
        }
    }
    else
    {
        // Single file upload
        const std::string& data = req.body();
        uploadStatus = saveConfigFile(data, fileID, asyncResp);
        if (!uploadStatus)
        {
            BMCWEB_LOG_INFO("ConfigFile upload failed!! FileName: {}", fileID);
            return;
        }
        BMCWEB_LOG_INFO("ConfigFile upload complete!! Filename: {}", fileID);
    }
}

inline void
    handleConfigFileList(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::vector<std::string> pathObjList;
    std::filesystem::path loc(configFilePath);
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        for (const auto& file : std::filesystem::directory_iterator(loc))
        {
            const std::filesystem::path& pathObj = file.path();
            if (std::filesystem::is_regular_file(pathObj))
            {
                pathObjList.push_back("/ibm/v1/Host/ConfigFiles/" +
                                      pathObj.filename().string());
            }
        }
    }
    asyncResp->res.jsonValue["@odata.type"] =
        "#IBMConfigFile.v1_0_0.IBMConfigFile";
    asyncResp->res.jsonValue["@odata.id"] = "/ibm/v1/Host/ConfigFiles/";
    asyncResp->res.jsonValue["Id"] = "ConfigFiles";
    asyncResp->res.jsonValue["Name"] = "ConfigFiles";

    asyncResp->res.jsonValue["Members"] = std::move(pathObjList);
    asyncResp->res.jsonValue["Actions"]["#IBMConfigFiles.DeleteAll"] = {
        {"target",
         "/ibm/v1/Host/ConfigFiles/Actions/IBMConfigFiles.DeleteAll"}};
}

inline void
    deleteConfigFiles(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    std::error_code ec;
    std::filesystem::path loc(configFilePath);
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        std::filesystem::remove_all(loc, ec);
        if (ec)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            BMCWEB_LOG_ERROR("deleteConfigFiles: Failed to delete the "
                             "config files directory. ec : {}",
                             ec.message());
        }
        BMCWEB_LOG_CRITICAL(
            "INFO:config files directory delete successful. PATH: {}",
            loc.string());
        std::string origin = "/ibm/v1/Host/ConfigFiles";
        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceRemoved(), origin, "IBMConfigFile");
    }
}

inline void
    getLockServiceData(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    asyncResp->res.jsonValue["@odata.type"] = "#LockService.v1_0_0.LockService";
    asyncResp->res.jsonValue["@odata.id"] = "/ibm/v1/HMC/LockService/";
    asyncResp->res.jsonValue["Id"] = "LockService";
    asyncResp->res.jsonValue["Name"] = "LockService";

    asyncResp->res.jsonValue["Actions"]["#LockService.AcquireLock"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock"}};
    asyncResp->res.jsonValue["Actions"]["#LockService.ReleaseLock"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock"}};
    asyncResp->res.jsonValue["Actions"]["#LockService.GetLockList"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList"}};
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

    std::string contentDispositionParam = "attachment; filename=\"" + fileID +
                                          "\"";
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
    std::string filePath("/var/lib/bmcweb/ibm-management-console/configfiles/" +
                         fileID);
    BMCWEB_LOG_DEBUG("Removing the file : {}", filePath);
    std::ifstream fileOpen(filePath.c_str());
    if (static_cast<bool>(fileOpen))
    {
        if (remove(filePath.c_str()) == 0)
        {
            BMCWEB_LOG_CRITICAL("INFO:configFile removed, FilePath: {}",
                                filePath);
            asyncResp->res.jsonValue["Description"] = "File Deleted";
            std::string origin = "/ibm/v1/Host/ConfigFiles/" + fileID;
            redfish::EventServiceManager::getInstance().sendEvent(
                redfish::messages::resourceRemoved(), origin, "IBMConfigFile");
        }
        else
        {
            BMCWEB_LOG_ERROR("File not removed, FilePath: {}", filePath);
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR("File not found, FilePath: {}", filePath);
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

    std::string origin = "/ibm/v1/HMC/BroadcastService";
    nlohmann::json msgJson = {{"Message", broadcastMsg}};

    redfish::EventServiceManager::getInstance().sendEvent(msgJson, origin,
                                                          "BroadcastService");
}

inline void handleFileUrl(const crow::Request& req,
                          const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                          const std::string& fileID)
{
    if (req.method() == boost::beast::http::verb::put)
    {
        handleFileUpload(req, asyncResp, fileID);
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

inline void
    handleFileUrlPost(crow::App& app, const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    if (!redfish::setUpRedfishRoute(app, req, asyncResp))
    {
        return;
    }
    handleFileUpload(req, asyncResp);
}

inline void
    handleAcquireLockAPI(const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         std::vector<nlohmann::json> body)
{
    LockRequests lockRequestStructure;
    for (auto& element : body)
    {
        std::string lockType;
        uint64_t resourceId = 0;

        SegmentFlags segInfo;
        std::vector<nlohmann::json> segmentFlags;

        if (!redfish::json_util::readJson(element, asyncResp->res, "LockType",
                                          lockType, "ResourceID", resourceId,
                                          "SegmentFlags", segmentFlags))
        {
            BMCWEB_LOG_DEBUG("Not a Valid JSON");
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        BMCWEB_LOG_DEBUG("{}", lockType);
        BMCWEB_LOG_DEBUG("{}", resourceId);

        BMCWEB_LOG_DEBUG("Segment Flags are present");

        for (auto& e : segmentFlags)
        {
            std::string lockFlags;
            uint32_t segmentLength = 0;

            if (!redfish::json_util::readJson(e, asyncResp->res, "LockFlag",
                                              lockFlags, "SegmentLength",
                                              segmentLength))
            {
                asyncResp->res.result(boost::beast::http::status::bad_request);
                return;
            }

            BMCWEB_LOG_DEBUG("Lockflag : {}", lockFlags);
            BMCWEB_LOG_DEBUG("SegmentLength : {}", segmentLength);

            segInfo.emplace_back(lockFlags, segmentLength);
        }

        lockRequestStructure.emplace_back(req.session->uniqueId,
                                          req.session->clientId.value_or(""),
                                          lockType, resourceId, segInfo);
    }

    // print lock request into journal

    for (auto& i : lockRequestStructure)
    {
        BMCWEB_LOG_DEBUG("{}", std::get<0>(i));
        BMCWEB_LOG_DEBUG("{}", std::get<1>(i));
        BMCWEB_LOG_DEBUG("{}", std::get<2>(i));
        BMCWEB_LOG_DEBUG("{}", std::get<3>(i));

        for (const auto& p : std::get<4>(i))
        {
            BMCWEB_LOG_DEBUG("{}, {}", p.first, p.second);
        }
    }

    const LockRequests& t = lockRequestStructure;

    auto varAcquireLock = crow::ibm_mc_lock::Lock::getInstance().acquireLock(t);

    if (varAcquireLock.first)
    {
        // Either validity failure or there is a conflict with itself

        auto validityStatus =
            std::get<std::pair<bool, int>>(varAcquireLock.second);

        if ((!validityStatus.first) && (validityStatus.second == 0))
        {
            BMCWEB_LOG_DEBUG("Not a Valid record");
            BMCWEB_LOG_DEBUG("Bad json in request");
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        if (validityStatus.first && (validityStatus.second == 1))
        {
            BMCWEB_LOG_ERROR(
                "handleAcquireLockAPI: There is a conflict within itself");
            asyncResp->res.result(boost::beast::http::status::conflict);
            return;
        }
    }
    else
    {
        auto conflictStatus =
            std::get<crow::ibm_mc_lock::Rc>(varAcquireLock.second);
        if (!conflictStatus.first)
        {
            BMCWEB_LOG_DEBUG("There is no conflict with the locktable");
            asyncResp->res.result(boost::beast::http::status::ok);

            auto var = std::get<uint32_t>(conflictStatus.second);
            nlohmann::json returnJson;
            returnJson["id"] = var;
            asyncResp->res.jsonValue["TransactionID"] = var;
            return;
        }

        // Check if the session is active. If active, send back the lock
        // conflict, else clear the stale entry in the lock table.
        auto var =
            std::get<std::pair<uint32_t, LockRequest>>(conflictStatus.second);
        std::string sessionId = std::get<0>(var.second);
        auto session =
            persistent_data::SessionStore::getInstance().getSessionByUid(
                sessionId);

        if (session == nullptr)
        {
            BMCWEB_LOG_ERROR("Releasing lock of the session id(stale): {}",
                             sessionId);
            // clear the stale entry
            crow::ibm_mc_lock::Lock::getInstance().releaseLock(sessionId);

            // create the new lock record for the session
            varAcquireLock =
                crow::ibm_mc_lock::Lock::getInstance().acquireLock(t);
            conflictStatus =
                std::get<crow::ibm_mc_lock::Rc>(varAcquireLock.second);
            if (!conflictStatus.first)
            {
                asyncResp->res.result(boost::beast::http::status::ok);

                auto id = std::get<uint32_t>(conflictStatus.second);
                nlohmann::json returnJson;
                returnJson["id"] = id;
                asyncResp->res.jsonValue["TransactionID"] = id;
                return;
            }
        }

        BMCWEB_LOG_ERROR(
            "handleAcquireLockAPI: There is a conflict with the lock table");

        asyncResp->res.result(boost::beast::http::status::conflict);
        nlohmann::json returnJson;
        nlohmann::json segments;
        nlohmann::json myarray = nlohmann::json::array();
        returnJson["TransactionID"] = var.first;
        returnJson["SessionID"] = sessionId;
        returnJson["HMCID"] = std::get<1>(var.second);
        returnJson["LockType"] = std::get<2>(var.second);
        returnJson["ResourceID"] = std::get<3>(var.second);

        for (const auto& i : std::get<4>(var.second))
        {
            segments["LockFlag"] = i.first;
            segments["SegmentLength"] = i.second;
            myarray.push_back(segments);
        }

        returnJson["SegmentFlags"] = myarray;
        BMCWEB_LOG_ERROR("handleAcquireLockAPI: Conflicting lock record: {} ",
                         returnJson);
        asyncResp->res.jsonValue["Record"] = returnJson;
        return;
    }
}
inline void
    handleRelaseAllAPI(const crow::Request& req,
                       const std::shared_ptr<bmcweb::AsyncResp>& asyncResp)
{
    crow::ibm_mc_lock::Lock::getInstance().releaseLock(req.session->uniqueId);
    asyncResp->res.result(boost::beast::http::status::ok);
}

inline void
    handleReleaseLockAPI(const crow::Request& req,
                         const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const std::vector<uint32_t>& listTransactionIds)
{
    BMCWEB_LOG_DEBUG("{}", listTransactionIds.size());
    BMCWEB_LOG_DEBUG("Data is present");
    for (unsigned int listTransactionId : listTransactionIds)
    {
        BMCWEB_LOG_DEBUG("{}", listTransactionId);
    }

    // validate the request ids

    auto varReleaselock = crow::ibm_mc_lock::Lock::getInstance().releaseLock(
        listTransactionIds, std::make_pair(req.session->clientId.value_or(""),
                                           req.session->uniqueId));

    if (!varReleaselock.first)
    {
        // validation Failed
        BMCWEB_LOG_ERROR("handleReleaseLockAPI: validation failed");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }
    auto statusRelease =
        std::get<crow::ibm_mc_lock::RcRelaseLock>(varReleaselock.second);
    if (statusRelease.first)
    {
        // The current hmc owns all the locks, so we already released
        // them
        return;
    }

    // valid rid, but the current hmc does not own all the locks
    BMCWEB_LOG_DEBUG(
        "handleReleaseLockAPI: Current HMC does not own all the locks");
    asyncResp->res.result(boost::beast::http::status::unauthorized);

    auto var = statusRelease.second;
    nlohmann::json returnJson;
    nlohmann::json segments;
    nlohmann::json myArray = nlohmann::json::array();
    returnJson["TransactionID"] = var.first;
    returnJson["SessionID"] = std::get<0>(var.second);
    returnJson["HMCID"] = std::get<1>(var.second);
    returnJson["LockType"] = std::get<2>(var.second);
    returnJson["ResourceID"] = std::get<3>(var.second);

    for (const auto& i : std::get<4>(var.second))
    {
        segments["LockFlag"] = i.first;
        segments["SegmentLength"] = i.second;
        myArray.push_back(segments);
    }

    returnJson["SegmentFlags"] = myArray;
    BMCWEB_LOG_DEBUG("handleReleaseLockAPI: lockrecord: {} ", returnJson);
    asyncResp->res.jsonValue["Record"] = returnJson;
}

inline void
    handleGetLockListAPI(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                         const ListOfSessionIds& listSessionIds)
{
    BMCWEB_LOG_DEBUG("{}", listSessionIds.size());

    auto status =
        crow::ibm_mc_lock::Lock::getInstance().getLockList(listSessionIds);
    auto var = std::get<std::vector<std::pair<uint32_t, LockRequests>>>(status);

    nlohmann::json lockRecords = nlohmann::json::array();

    for (const auto& transactionId : var)
    {
        for (const auto& lockRecord : transactionId.second)
        {
            nlohmann::json returnJson;

            returnJson["TransactionID"] = transactionId.first;
            returnJson["SessionID"] = std::get<0>(lockRecord);
            returnJson["HMCID"] = std::get<1>(lockRecord);
            returnJson["LockType"] = std::get<2>(lockRecord);
            returnJson["ResourceID"] = std::get<3>(lockRecord);

            nlohmann::json segments;
            nlohmann::json segmentInfoArray = nlohmann::json::array();

            for (const auto& segment : std::get<4>(lockRecord))
            {
                segments["LockFlag"] = segment.first;
                segments["SegmentLength"] = segment.second;
                segmentInfoArray.push_back(segments);
            }

            returnJson["SegmentFlags"] = segmentInfoArray;
            lockRecords.push_back(returnJson);
        }
    }
    asyncResp->res.result(boost::beast::http::status::ok);
    asyncResp->res.jsonValue["Records"] = lockRecords;
}

inline void
    deleteVMIDbusEntry(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                       const std::string& entryId)
{
    auto respHandler = [asyncResp,
                        entryId](const boost::system::error_code ec) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("deleteVMIDbusEntry respHandler got error {}",
                             ec.message());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }
    };
    crow::connections::systemBus->async_method_call(
        respHandler, "xyz.openbmc_project.Certs.ca.authority.Manager",
        "/xyz/openbmc_project/certs/ca/entry/" + entryId,
        "xyz.openbmc_project.Object.Delete", "Delete");
}

inline void
    getCSREntryCertificate(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& entryId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp](const boost::system::error_code ec,
                    const std::variant<std::string>& certificate) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("getCSREntryCertificate respHandler got error: {}",
                             ec.message());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] =
                "Failed to get request status";
            return;
        }

        const std::string* cert = std::get_if<std::string>(&certificate);
        if (cert == nullptr)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] =
                "Failed to get certificate from host";
            return;
        }

        if (cert->empty())
        {
            BMCWEB_LOG_ERROR("Empty Client Certificate");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] =
                "Empty Client Certificate";
            return;
        }

        BMCWEB_LOG_CRITICAL("INFO:Successfully got VMI client certificate");
        asyncResp->res.jsonValue["Certificate"] = *cert;
    },
        "xyz.openbmc_project.Certs.ca.authority.Manager",
        "/xyz/openbmc_project/certs/ca/entry/" + entryId,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Certs.Entry", "ClientCertificate");
}

inline void getCSREntryAck(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                           const std::string& entryId)
{
    crow::connections::systemBus->async_method_call(
        [asyncResp, entryId](const boost::system::error_code ec,
                             const std::variant<std::string>& hostAck) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("getCSREntryAck respHandler got error: {}",
                             ec.message());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] =
                "Failed to get request status";
            return;
        }

        const std::string* status = std::get_if<std::string>(&hostAck);
        if (status == nullptr)
        {
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] =
                "Failed to get host ack status";
            return;
        }

        BMCWEB_LOG_CRITICAL(
            "INFO:VMI Cert Entry {} new status property value {}", entryId,
            *status);
        if (*status == "xyz.openbmc_project.Certs.Entry.State.Pending")
        {
            asyncResp->res.addHeader("Retry-After", "60");
            asyncResp->res.result(
                boost::beast::http::status::service_unavailable);
            asyncResp->res.jsonValue["Description"] =
                "Host is not ready to serve the request";
        }
        else if (*status == "xyz.openbmc_project.Certs.Entry.State.BadCSR")
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            asyncResp->res.jsonValue["Description"] = "Bad CSR";
            BMCWEB_LOG_CRITICAL("SignCSR failed with Bad CSR");
        }
        else if (*status == "xyz.openbmc_project.Certs.Entry.State.Complete")
        {
            getCSREntryCertificate(asyncResp, entryId);
        }

        deleteVMIDbusEntry(asyncResp, entryId);
        auto entry = ackMatches.find(entryId);
        if (entry != ackMatches.end())
        {
            BMCWEB_LOG_CRITICAL("INFO:Erasing match of entryId: {}", entryId);
            ackMatches.erase(entryId);
        }
    },
        "xyz.openbmc_project.Certs.ca.authority.Manager",
        "/xyz/openbmc_project/certs/ca/entry/" + entryId,
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.Certs.Entry", "Status");
}

static void
    handleCsrRequest(const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                     const std::string& csrString)
{
    std::shared_ptr<boost::asio::steady_timer> timeout =
        std::make_shared<boost::asio::steady_timer>(
            crow::connections::systemBus->get_io_context());

    timeout->expires_after(std::chrono::seconds(30));
    crow::connections::systemBus->async_method_call(
        [asyncResp, timeout](const boost::system::error_code ec,
                             sdbusplus::message::message& m) {
        sdbusplus::message::object_path objPath;
        m.read(objPath);
        if (ec)
        {
            BMCWEB_LOG_ERROR("Error in creating CSR Entry object : {} ",
                             ec.message());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            return;
        }

        std::string entryId = objPath.filename();
        if (entryId.empty())
        {
            BMCWEB_LOG_ERROR("Invalid ID in objPath");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            return;
        }

        BMCWEB_LOG_INFO("Created CSR Entry object with entryId: {}", entryId);
        auto timeoutHandler = [asyncResp, timeout, entryId](
                                  const boost::system::error_code errorCode) {
            if (errorCode)
            {
                if (errorCode != boost::asio::error::operation_aborted)
                {
                    BMCWEB_LOG_ERROR("Async_wait failed {}", errorCode);
                }
                return;
            }

            getCSREntryAck(asyncResp, entryId);
            timeout->cancel();
        };

        timeout->async_wait(timeoutHandler);

        auto callback = [asyncResp, timeout,
                         entryId](sdbusplus::message::message& msg) {
            boost::container::flat_map<std::string, std::variant<std::string>>
                values;
            std::string iface;
            msg.read(iface, values);
            if (iface == "xyz.openbmc_project.Certs.Entry")
            {
                auto findStatus = values.find("Status");
                if (findStatus != values.end())
                {
                    BMCWEB_LOG_CRITICAL(
                        "INFO:Found status prop change of VMI cert object with entryId: {}",
                        entryId);
                    getCSREntryAck(asyncResp, entryId);
                    timeout->cancel();
                }
            }
        };

        std::string matchStr(
            "interface='org.freedesktop.DBus.Properties',type='"
            "signal',"
            "member='PropertiesChanged',path='/xyz/openbmc_project/certs/"
            "ca/entry/" +
            entryId + "'");

        ackMatches.emplace(
            entryId, std::make_unique<sdbusplus::bus::match::match>(
                         *crow::connections::systemBus, matchStr, callback));
    },
        "xyz.openbmc_project.Certs.ca.authority.Manager",
        "/xyz/openbmc_project/certs/ca", "xyz.openbmc_project.Certs.Authority",
        "SignCSR", csrString);
}

inline void
    handlePassThrough(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const std::string& name)
{
    std::vector<int32_t> command;
    if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "Send",
                                           command))
    {
        BMCWEB_LOG_DEBUG("Not a Valid JSON");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    if (command.empty())
    {
        BMCWEB_LOG_ERROR("Command is empty");
        asyncResp->res.result(boost::beast::http::status::bad_request);
        return;
    }

    auto respHandler = [asyncResp](const boost::system::error_code ec,
                                   const std::vector<int32_t>& resp) {
        if (ec)
        {
            BMCWEB_LOG_ERROR("handlePassThrough respHandler got error, ec = {}",
                             ec.value());
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            return;
        }

        if (resp.empty())
        {
            redfish::messages::internalError(asyncResp->res);
            return;
        }

        std::string strData = "ai " + std::to_string(resp.size());
        for (const auto& value : resp)
        {
            strData.append(" ");
            strData.append(std::to_string(value));
        }

        asyncResp->res.addHeader("Content-Type", "application/octet-stream");
        asyncResp->res.write(strData.c_str());
    };

    crow::connections::systemBus->async_method_call(
        respHandler, "org.open_power.OCC.Control",
        "/org/open_power/control/" + name, "org.open_power.OCC.PassThrough",
        "Send", command);
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
        asyncResp->res.jsonValue["LockService"]["@odata.id"] =
            "/ibm/v1/HMC/LockService";
        asyncResp->res.jsonValue["BroadcastService"]["@odata.id"] =
            "/ibm/v1/HMC/BroadcastService";
        asyncResp->res.jsonValue["Certificate"]["@odata.id"] =
            "/ibm/v1/Host/Certificate";
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
        if (!isValidConfigFileName(fileName, asyncResp->res.jsonValue))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        handleFileUrl(req, asyncResp, fileName);
    });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Certificate")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.jsonValue["@odata.id"] = "/ibm/v1/Host/Certificate";
        asyncResp->res.jsonValue["Id"] = "Certificate";
        asyncResp->res.jsonValue["Name"] = "Certificate";
        asyncResp->res.jsonValue["Actions"]["SignCSR"] = {
            {"target", "/ibm/v1/Host/Actions/SignCSR"}};
        asyncResp->res.jsonValue["root"] = {
            {"target", "/ibm/v1/Host/Certificate/root"}};
    });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Certificate/root")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        if (!std::filesystem::exists(rootCertPath))
        {
            BMCWEB_LOG_ERROR("RootCert file does not exist");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            return;
        }
        std::ifstream inFile;
        inFile.open(rootCertPath.c_str());
        if (inFile.fail())
        {
            BMCWEB_LOG_DEBUG("Error while opening the root certificate "
                             "file for reading");
            asyncResp->res.result(
                boost::beast::http::status::internal_server_error);
            asyncResp->res.jsonValue["Description"] = internalServerError;
            return;
        }

        std::stringstream strStream;
        strStream << inFile.rdbuf();
        inFile.close();
        asyncResp->res.jsonValue["Certificate"] = strStream.str();
    });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Actions/SignCSR")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        std::string csrString;
        if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "CsrString",
                                               csrString))
        {
            return;
        }

        handleCsrRequest(asyncResp, csrString);
    });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            std::bind_front(handleFileUrlPost, std::ref(app)));

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        getLockServiceData(asyncResp);
    });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        std::vector<nlohmann::json> body;
        if (!redfish::json_util::readJsonAction(req, asyncResp->res, "Request",
                                                body))
        {
            BMCWEB_LOG_DEBUG("Not a Valid JSON");
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        handleAcquireLockAPI(req, asyncResp, body);
    });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        std::string type;
        std::vector<uint32_t> listTransactionIds;

        if (!redfish::json_util::readJsonPatch(req, asyncResp->res, "Type",
                                               type, "TransactionIDs",
                                               listTransactionIds))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        if (type == "Transaction")
        {
            handleReleaseLockAPI(req, asyncResp, listTransactionIds);
        }
        else if (type == "Session")
        {
            handleRelaseAllAPI(req, asyncResp);
        }
        else
        {
            BMCWEB_LOG_DEBUG(" Value of Type : {} is Not a Valid key", type);
            redfish::messages::propertyValueNotInList(asyncResp->res, type,
                                                      "Type");
        }
    });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        ListOfSessionIds listSessionIds;

        if (!redfish::json_util::readJsonPatch(req, asyncResp->res,
                                               "SessionIDs", listSessionIds))
        {
            asyncResp->res.result(boost::beast::http::status::bad_request);
            return;
        }
        handleGetLockListAPI(asyncResp, listSessionIds);
    });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/BroadcastService")
        .privileges({{"ConfigureComponents", "ConfigureManager"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        handleBroadcastService(req, asyncResp);
    });

    BMCWEB_ROUTE(app, "/ibm/v1/OCC/Control/<str>/Actions/PassThrough.Send")
        .privileges({{"OemIBMPerformService"}})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req,
               const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
               const std::string& name) {
        handlePassThrough(req, asyncResp, name);
    });
}

} // namespace ibm_mc
} // namespace crow
