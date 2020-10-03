#pragma once
#include <tinyxml2.h>

#include <app.hpp>
#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <error_messages.hpp>
#include <event_service_manager.hpp>
#include <ibm/locks.hpp>
#include <nlohmann/json.hpp>
#include <resource_messages.hpp>
#include <sdbusplus/message/types.hpp>
#include <utils/json_utils.hpp>

#include <filesystem>
#include <fstream>
#include <regex>

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

constexpr size_t maxSaveareaFileSize =
    500000; // Allow save area file size upto 500KB
constexpr size_t maxBroadcastMsgSize =
    1000; // Allow Broadcast message size upto 1KB

inline bool createSaveAreaPath(crow::Response& res)
{
    // The path /var/lib/obmc will be created by initrdscripts
    // Create the directories for the save-area files, when we get
    // first file upload request
    std::error_code ec;
    if (!std::filesystem::is_directory("/var/lib/obmc/bmc-console-mgmt", ec))
    {
        std::filesystem::create_directory("/var/lib/obmc/bmc-console-mgmt", ec);
    }
    if (ec)
    {
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = internalServerError;
        BMCWEB_LOG_DEBUG
            << "handleIbmPost: Failed to prepare save-area directory. ec : "
            << ec;
        return false;
    }

    if (!std::filesystem::is_directory(
            "/var/lib/obmc/bmc-console-mgmt/save-area", ec))
    {
        std::filesystem::create_directory(
            "/var/lib/obmc/bmc-console-mgmt/save-area", ec);
    }
    if (ec)
    {
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = internalServerError;
        BMCWEB_LOG_DEBUG
            << "handleIbmPost: Failed to prepare save-area directory. ec : "
            << ec;
        return false;
    }
    return true;
}

inline void handleFilePut(const crow::Request& req, crow::Response& res,
                          const std::string& fileID)
{
    // Check the content-type of the request
    std::string_view contentType = req.getHeaderValue("content-type");
    if (boost::starts_with(contentType, "multipart/form-data"))
    {
        BMCWEB_LOG_DEBUG
            << "This is multipart/form-data. Invalid content for PUT";

        res.result(boost::beast::http::status::not_acceptable);
        res.jsonValue["Description"] = contentNotAcceptableMsg;
        return;
    }
    BMCWEB_LOG_DEBUG << "Not a multipart/form-data. Continue..";

    BMCWEB_LOG_DEBUG
        << "handleIbmPut: Request to create/update the save-area file";
    if (!createSaveAreaPath(res))
    {
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }
    // Create the file
    std::ofstream file;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    loc /= fileID;

    const std::string& data = req.body;
    BMCWEB_LOG_DEBUG << "data capaticty : " << data.capacity();
    if (data.capacity() > maxSaveareaFileSize)
    {
        res.result(boost::beast::http::status::bad_request);
        res.jsonValue["Description"] =
            "File size exceeds maximum allowed size[500KB]";
        return;
    }
    BMCWEB_LOG_DEBUG << "Writing to the file: " << loc;

    bool fileExists = false;
    if (std::filesystem::exists(loc))
    {
        fileExists = true;
    }
    file.open(loc, std::ofstream::out);
    if (file.fail())
    {
        BMCWEB_LOG_DEBUG << "Error while opening the file for writing";
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = "Error while creating the file";
        return;
    }
    file << data;
    std::string origin = "/ibm/v1/Host/ConfigFiles/" + fileID;
    // Push an event
    if (fileExists)
    {
        BMCWEB_LOG_DEBUG << "config file is updated";
        res.jsonValue["Description"] = "File Updated";

        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceChanged(), origin, "IBMConfigFile");
    }
    else
    {
        BMCWEB_LOG_DEBUG << "config file is created";
        res.jsonValue["Description"] = "File Created";

        redfish::EventServiceManager::getInstance().sendEvent(
            redfish::messages::resourceCreated(), origin, "IBMConfigFile");
    }
}

inline void handleConfigFileList(crow::Response& res)
{
    std::vector<std::string> pathObjList;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        for (const auto& file : std::filesystem::directory_iterator(loc))
        {
            const std::filesystem::path& pathObj = file.path();
            pathObjList.push_back("/ibm/v1/Host/ConfigFiles/" +
                                  pathObj.filename().string());
        }
    }
    res.jsonValue["@odata.type"] = "#IBMConfigFile.v1_0_0.IBMConfigFile";
    res.jsonValue["@odata.id"] = "/ibm/v1/Host/ConfigFiles/";
    res.jsonValue["Id"] = "ConfigFiles";
    res.jsonValue["Name"] = "ConfigFiles";

    res.jsonValue["Members"] = std::move(pathObjList);
    res.jsonValue["Actions"]["#IBMConfigFiles.DeleteAll"] = {
        {"target",
         "/ibm/v1/Host/ConfigFiles/Actions/IBMConfigFiles.DeleteAll"}};
    res.end();
}

inline void deleteConfigFiles(crow::Response& res)
{
    std::vector<std::string> pathObjList;
    std::error_code ec;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        std::filesystem::remove_all(loc, ec);
        if (ec)
        {
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue["Description"] = internalServerError;
            BMCWEB_LOG_DEBUG << "deleteConfigFiles: Failed to delete the "
                                "config files directory. ec : "
                             << ec;
        }
    }
    res.end();
}

inline void getLockServiceData(crow::Response& res)
{
    res.jsonValue["@odata.type"] = "#LockService.v1_0_0.LockService";
    res.jsonValue["@odata.id"] = "/ibm/v1/HMC/LockService/";
    res.jsonValue["Id"] = "LockService";
    res.jsonValue["Name"] = "LockService";

    res.jsonValue["Actions"]["#LockService.AcquireLock"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock"}};
    res.jsonValue["Actions"]["#LockService.ReleaseLock"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock"}};
    res.jsonValue["Actions"]["#LockService.GetLockList"] = {
        {"target", "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList"}};
    res.end();
}

inline void handleFileGet(crow::Response& res, const std::string& fileID)
{
    BMCWEB_LOG_DEBUG << "HandleGet on SaveArea files on path: " << fileID;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area/" +
                              fileID);
    if (!std::filesystem::exists(loc))
    {
        BMCWEB_LOG_ERROR << loc << "Not found";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    std::ifstream readfile(loc.string());
    if (!readfile)
    {
        BMCWEB_LOG_ERROR << loc.string() << "Not found";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    std::string contentDispositionParam =
        "attachment; filename=\"" + fileID + "\"";
    res.addHeader("Content-Disposition", contentDispositionParam);
    std::string fileData;
    fileData = {std::istreambuf_iterator<char>(readfile),
                std::istreambuf_iterator<char>()};
    res.jsonValue["Data"] = fileData;
    return;
}

inline void handleFileDelete(crow::Response& res, const std::string& fileID)
{
    std::string filePath("/var/lib/obmc/bmc-console-mgmt/save-area/" + fileID);
    BMCWEB_LOG_DEBUG << "Removing the file : " << filePath << "\n";

    std::ifstream fileOpen(filePath.c_str());
    if (static_cast<bool>(fileOpen))
    {
        if (remove(filePath.c_str()) == 0)
        {
            BMCWEB_LOG_DEBUG << "File removed!\n";
            res.jsonValue["Description"] = "File Deleted";
        }
        else
        {
            BMCWEB_LOG_ERROR << "File not removed!\n";
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue["Description"] = internalServerError;
        }
    }
    else
    {
        BMCWEB_LOG_ERROR << "File not found!\n";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
    }
    return;
}

inline void handleBroadcastService(const crow::Request& req,
                                   crow::Response& res)
{
    std::string broadcastMsg;

    if (!redfish::json_util::readJson(req, res, "Message", broadcastMsg))
    {
        BMCWEB_LOG_DEBUG << "Not a Valid JSON";
        res.result(boost::beast::http::status::bad_request);
        return;
    }
    if (broadcastMsg.size() > maxBroadcastMsgSize)
    {
        BMCWEB_LOG_ERROR << "Message size exceeds maximum allowed size[1KB]";
        res.result(boost::beast::http::status::bad_request);
        return;
    }
    redfish::EventServiceManager::getInstance().sendBroadcastMsg(broadcastMsg);
    res.end();
    return;
}

inline void handleFileUrl(const crow::Request& req, crow::Response& res,
                          const std::string& fileID)
{
    if (req.method() == boost::beast::http::verb::put)
    {
        handleFilePut(req, res, fileID);
        res.end();
        return;
    }
    if (req.method() == boost::beast::http::verb::get)
    {
        handleFileGet(res, fileID);
        res.end();
        return;
    }
    if (req.method() == boost::beast::http::verb::delete_)
    {
        handleFileDelete(res, fileID);
        res.end();
        return;
    }
}

inline void handleAcquireLockAPI(const crow::Request& req, crow::Response& res,
                                 std::vector<nlohmann::json> body)
{
    LockRequests lockRequestStructure;
    for (auto& element : body)
    {
        std::string lockType;
        uint64_t resourceId;

        SegmentFlags segInfo;
        std::vector<nlohmann::json> segmentFlags;

        if (!redfish::json_util::readJson(element, res, "LockType", lockType,
                                          "ResourceID", resourceId,
                                          "SegmentFlags", segmentFlags))
        {
            BMCWEB_LOG_DEBUG << "Not a Valid JSON";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
        BMCWEB_LOG_DEBUG << lockType;
        BMCWEB_LOG_DEBUG << resourceId;

        BMCWEB_LOG_DEBUG << "Segment Flags are present";

        for (auto& e : segmentFlags)
        {
            std::string lockFlags;
            uint32_t segmentLength;

            if (!redfish::json_util::readJson(e, res, "LockFlag", lockFlags,
                                              "SegmentLength", segmentLength))
            {
                res.result(boost::beast::http::status::bad_request);
                res.end();
                return;
            }

            BMCWEB_LOG_DEBUG << "Lockflag : " << lockFlags;
            BMCWEB_LOG_DEBUG << "SegmentLength : " << segmentLength;

            segInfo.push_back(std::make_pair(lockFlags, segmentLength));
        }
        lockRequestStructure.push_back(
            make_tuple(req.session->uniqueId, req.session->clientId, lockType,
                       resourceId, segInfo));
    }

    // print lock request into journal

    for (auto& i : lockRequestStructure)
    {
        BMCWEB_LOG_DEBUG << std::get<0>(i);
        BMCWEB_LOG_DEBUG << std::get<1>(i);
        BMCWEB_LOG_DEBUG << std::get<2>(i);
        BMCWEB_LOG_DEBUG << std::get<3>(i);

        for (const auto& p : std::get<4>(i))
        {
            BMCWEB_LOG_DEBUG << p.first << ", " << p.second;
        }
    }

    const LockRequests& t = lockRequestStructure;

    auto varAcquireLock = crow::ibm_mc_lock::Lock::getInstance().acquireLock(t);

    if (varAcquireLock.first)
    {
        // Either validity failure of there is a conflict with itself

        auto validityStatus =
            std::get<std::pair<bool, int>>(varAcquireLock.second);

        if ((!validityStatus.first) && (validityStatus.second == 0))
        {
            BMCWEB_LOG_DEBUG << "Not a Valid record";
            BMCWEB_LOG_DEBUG << "Bad json in request";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
        if (validityStatus.first && (validityStatus.second == 1))
        {
            BMCWEB_LOG_DEBUG << "There is a conflict within itself";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
    }
    else
    {
        auto conflictStatus =
            std::get<crow::ibm_mc_lock::Rc>(varAcquireLock.second);
        if (!conflictStatus.first)
        {
            BMCWEB_LOG_DEBUG << "There is no conflict with the locktable";
            res.result(boost::beast::http::status::ok);

            auto var = std::get<uint32_t>(conflictStatus.second);
            nlohmann::json returnJson;
            returnJson["id"] = var;
            res.jsonValue["TransactionID"] = var;
            res.end();
            return;
        }
        BMCWEB_LOG_DEBUG << "There is a conflict with the lock table";
        res.result(boost::beast::http::status::conflict);
        auto var =
            std::get<std::pair<uint32_t, LockRequest>>(conflictStatus.second);
        nlohmann::json returnJson, segments;
        nlohmann::json myarray = nlohmann::json::array();
        returnJson["TransactionID"] = var.first;
        returnJson["SessionID"] = std::get<0>(var.second);
        returnJson["HMCID"] = std::get<1>(var.second);
        returnJson["LockType"] = std::get<2>(var.second);
        returnJson["ResourceID"] = std::get<3>(var.second);

        for (auto& i : std::get<4>(var.second))
        {
            segments["LockFlag"] = i.first;
            segments["SegmentLength"] = i.second;
            myarray.push_back(segments);
        }

        returnJson["SegmentFlags"] = myarray;

        res.jsonValue["Record"] = returnJson;
        res.end();
        return;
    }
}
inline void handleRelaseAllAPI(const crow::Request& req, crow::Response& res)
{
    crow::ibm_mc_lock::Lock::getInstance().releaseLock(req.session->uniqueId);
    res.result(boost::beast::http::status::ok);
    res.end();
    return;
}

inline void
    handleReleaseLockAPI(const crow::Request& req, crow::Response& res,
                         const std::vector<uint32_t>& listTransactionIds)
{
    BMCWEB_LOG_DEBUG << listTransactionIds.size();
    BMCWEB_LOG_DEBUG << "Data is present";
    for (unsigned int listTransactionId : listTransactionIds)
    {
        BMCWEB_LOG_DEBUG << listTransactionId;
    }

    // validate the request ids

    auto varReleaselock = crow::ibm_mc_lock::Lock::getInstance().releaseLock(
        listTransactionIds,
        std::make_pair(req.session->clientId, req.session->uniqueId));

    if (!varReleaselock.first)
    {
        // validation Failed
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    auto statusRelease =
        std::get<crow::ibm_mc_lock::RcRelaseLock>(varReleaselock.second);
    if (statusRelease.first)
    {
        // The current hmc owns all the locks, so we already released
        // them
        res.result(boost::beast::http::status::ok);
        res.end();
        return;
    }

    // valid rid, but the current hmc does not own all the locks
    BMCWEB_LOG_DEBUG << "Current HMC does not own all the locks";
    res.result(boost::beast::http::status::unauthorized);

    auto var = statusRelease.second;
    nlohmann::json returnJson, segments;
    nlohmann::json myArray = nlohmann::json::array();
    returnJson["TransactionID"] = var.first;
    returnJson["SessionID"] = std::get<0>(var.second);
    returnJson["HMCID"] = std::get<1>(var.second);
    returnJson["LockType"] = std::get<2>(var.second);
    returnJson["ResourceID"] = std::get<3>(var.second);

    for (auto& i : std::get<4>(var.second))
    {
        segments["LockFlag"] = i.first;
        segments["SegmentLength"] = i.second;
        myArray.push_back(segments);
    }

    returnJson["SegmentFlags"] = myArray;
    res.jsonValue["Record"] = returnJson;
    res.end();
    return;
}

inline void handleGetLockListAPI(crow::Response& res,
                                 const ListOfSessionIds& listSessionIds)
{
    BMCWEB_LOG_DEBUG << listSessionIds.size();

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
    res.result(boost::beast::http::status::ok);
    res.jsonValue["Records"] = lockRecords;
    res.end();
}

inline void requestRoutes(App& app)
{

    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                res.jsonValue["@odata.type"] =
                    "#ibmServiceRoot.v1_0_0.ibmServiceRoot";
                res.jsonValue["@odata.id"] = "/ibm/v1/";
                res.jsonValue["Id"] = "IBM Rest RootService";
                res.jsonValue["Name"] = "IBM Service Root";
                res.jsonValue["ConfigFiles"] = {
                    {"@odata.id", "/ibm/v1/Host/ConfigFiles"}};
                res.jsonValue["LockService"] = {
                    {"@odata.id", "/ibm/v1/HMC/LockService"}};
                res.jsonValue["BroadcastService"] = {
                    {"@odata.id", "/ibm/v1/HMC/BroadcastService"}};
                res.end();
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                handleConfigFileList(res);
            });

    BMCWEB_ROUTE(app,
                 "/ibm/v1/Host/ConfigFiles/Actions/IBMConfigFiles.DeleteAll")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request&, crow::Response& res) {
                deleteConfigFiles(res);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles/<path>")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::put, boost::beast::http::verb::get,
                 boost::beast::http::verb::delete_)(
            [](const crow::Request& req, crow::Response& res,
               const std::string& path) { handleFileUrl(req, res, path); });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::get)(
            [](const crow::Request&, crow::Response& res) {
                getLockServiceData(res);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req, crow::Response& res) {
                std::vector<nlohmann::json> body;
                if (!redfish::json_util::readJson(req, res, "Request", body))
                {
                    BMCWEB_LOG_DEBUG << "Not a Valid JSON";
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                handleAcquireLockAPI(req, res, body);
            });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.ReleaseLock")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::post)([](const crow::Request& req,
                                                    crow::Response& res) {
            std::string type;
            std::vector<uint32_t> listTransactionIds;

            if (!redfish::json_util::readJson(req, res, "Type", type,
                                              "TransactionIDs",
                                              listTransactionIds))
            {
                res.result(boost::beast::http::status::bad_request);
                res.end();
                return;
            }
            if (type == "Transaction")
            {
                handleReleaseLockAPI(req, res, listTransactionIds);
            }
            else if (type == "Session")
            {
                handleRelaseAllAPI(req, res);
            }
            else
            {
                BMCWEB_LOG_DEBUG << " Value of Type : " << type
                                 << "is Not a Valid key";
                redfish::messages::propertyValueNotInList(res, type, "Type");
            }
        });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req, crow::Response& res) {
                ListOfSessionIds listSessionIds;

                if (!redfish::json_util::readJson(req, res, "SessionIDs",
                                                  listSessionIds))
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                handleGetLockListAPI(res, listSessionIds);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/BroadcastService")
        .privileges({"ConfigureComponents", "ConfigureManager"})
        .methods(boost::beast::http::verb::post)(
            [](const crow::Request& req, crow::Response& res) {
                handleBroadcastService(req, res);
            });
}

} // namespace ibm_mc
} // namespace crow
