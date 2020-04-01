#pragma once
#include <app.h>
#include <libpldm/base.h>
#include <libpldm/file_io.h>
#include <libpldm/pldm.h>
#include <poll.h>
#include <sys/inotify.h>
#include <tinyxml2.h>

#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <ibm/locks.hpp>
#include <nlohmann/json.hpp>
#include <regex>
#include <sdbusplus/message/types.hpp>
#include <utils/json_utils.hpp>

#define MAX_SAVE_AREA_FILESIZE 200000
using stype = std::string;
using segmentflags = std::vector<std::pair<std::string, uint32_t>>;
using lockrequest = std::tuple<stype, stype, stype, uint64_t, segmentflags>;
using lockrequests = std::vector<lockrequest>;
using rc = std::pair<bool, std::variant<uint32_t, lockrequest>>;
using rcrelaselock = std::pair<bool, lockrequest>;
using rcgetlocklist = std::pair<
    bool,
    std::variant<std::string, std::vector<std::pair<uint32_t, lockrequests>>>>;

namespace crow
{
namespace ibm_mc
{
constexpr const char *methodNotAllowedMsg = "Method Not Allowed";
constexpr const char *resourceNotFoundMsg = "Resource Not Found";
constexpr const char *contentNotAcceptableMsg = "Content Not Acceptable";
constexpr const char *internalServerError = "Internal Server Error";
constexpr auto CLIENT_CERT_PATH = "/var/lib/bmcweb/ClientCert";
constexpr auto CSR_CERT_PATH = "/var/lib/bmcweb/CSR";
constexpr auto ROOT_CERT_PATH = "/var/lib/bmcweb/RootCert";
constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;
constexpr int pollTimeout = 5; // 5 seconds
bool CSRHandlerRunning = false;

/**
 * @brief Return codes from the pldm command response
 */
enum class ResponseStatus
{
    success,
    failure
};

bool createSaveAreaPath(crow::Response &res)
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
void handleFilePut(const crow::Request &req, crow::Response &res,
                   const std::string &fileID)
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
    else
    {
        BMCWEB_LOG_DEBUG << "Not a multipart/form-data. Continue..";
    }

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

    std::string data = std::move(req.body);
    BMCWEB_LOG_DEBUG << "data capaticty : " << data.capacity();
    if (data.capacity() > MAX_SAVE_AREA_FILESIZE)
    {
        res.result(boost::beast::http::status::bad_request);
        res.jsonValue["Description"] =
            "File size exceeds 200KB. Maximum allowed size is 200KB";
        return;
    }
    BMCWEB_LOG_DEBUG << "Creating file " << loc;
    file.open(loc, std::ofstream::out);
    if (file.fail())
    {
        BMCWEB_LOG_DEBUG << "Error while opening the file for writing";
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = "Error while creating the file";
        return;
    }
    else
    {
        file << data;
        BMCWEB_LOG_DEBUG << "save-area file is created";
        res.jsonValue["Description"] = "File Created";
    }
}

void handleConfigFileList(crow::Response &res)
{
    std::vector<std::string> pathObjList;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    if (std::filesystem::exists(loc) && std::filesystem::is_directory(loc))
    {
        for (const auto &file : std::filesystem::directory_iterator(loc))
        {
            std::filesystem::path pathObj(file.path());
            pathObjList.push_back("/ibm/v1/Host/ConfigFiles/" +
                                  pathObj.filename().string());
        }
    }
    res.jsonValue["@odata.type"] = "#FileCollection.v1_0_0.FileCollection";
    res.jsonValue["@odata.id"] = "/ibm/v1/Host/ConfigFiles/";
    res.jsonValue["Id"] = "ConfigFiles";
    res.jsonValue["Name"] = "ConfigFiles";

    res.jsonValue["Members"] = std::move(pathObjList);
    res.jsonValue["Actions"]["#FileCollection.DeleteAll"] = {
        {"target",
         "/ibm/v1/Host/ConfigFiles/Actions/FileCollection.DeleteAll"}};
    res.end();
}

void deleteConfigFiles(crow::Response &res)
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

void getLockServiceData(crow::Response &res)
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

void handleFileGet(crow::Response &res, const std::string &fileID)
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

void handleFileDelete(crow::Response &res, const std::string &fileID)
{
    std::string filePath("/var/lib/obmc/bmc-console-mgmt/save-area/" + fileID);
    BMCWEB_LOG_DEBUG << "Removing the file : " << filePath << "\n";

    std::ifstream file_open(filePath.c_str());
    if (static_cast<bool>(file_open))
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
    else
    {
        BMCWEB_LOG_ERROR << "File not found!\n";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
    }
    return;
}

inline void handleFileUrl(const crow::Request &req, crow::Response &res,
                          const std::string &fileID)
{
    if (req.method() == "PUT"_method)
    {
        handleFilePut(req, res, fileID);
        res.end();
        return;
    }
    if (req.method() == "GET"_method)
    {
        handleFileGet(res, fileID);
        res.end();
        return;
    }
    if (req.method() == "DELETE"_method)
    {
        handleFileDelete(res, fileID);
        res.end();
        return;
    }
}

void handleAcquireLockAPI(const crow::Request &req, crow::Response &res,
                          std::vector<nlohmann::json> body)
{
    lockrequests lockRequestStructure;
    for (auto element : body)
    {
        std::string lockType;
        uint64_t resourceId;

        segmentflags segInfo;
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

        for (auto e : segmentFlags)
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
        lockRequestStructure.push_back(make_tuple(
            req.session->uniqueId, "hmc-id", lockType, resourceId, segInfo));
    }

    // print lock request into journal

    for (uint32_t i = 0; i < lockRequestStructure.size(); i++)
    {
        BMCWEB_LOG_DEBUG << std::get<0>(lockRequestStructure[i]);
        BMCWEB_LOG_DEBUG << std::get<1>(lockRequestStructure[i]);
        BMCWEB_LOG_DEBUG << std::get<2>(lockRequestStructure[i]);
        BMCWEB_LOG_DEBUG << std::get<3>(lockRequestStructure[i]);

        for (const auto &p : std::get<4>(lockRequestStructure[i]))
        {
            BMCWEB_LOG_DEBUG << p.first << ", " << p.second;
        }
    }

    const lockrequests &t = lockRequestStructure;

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
            std::get<crow::ibm_mc_lock::rc>(varAcquireLock.second);
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
        else
        {
            BMCWEB_LOG_DEBUG << "There is a conflict with the lock table";
            res.result(boost::beast::http::status::conflict);
            auto var = std::get<std::pair<uint32_t, lockrequest>>(
                conflictStatus.second);
            nlohmann::json returnJson, segments;
            nlohmann::json myarray = nlohmann::json::array();
            returnJson["TransactionID"] = var.first;
            returnJson["SessionID"] = std::get<0>(var.second);
            returnJson["HMCID"] = std::get<1>(var.second);
            returnJson["LockType"] = std::get<2>(var.second);
            returnJson["ResourceID"] = std::get<3>(var.second);

            for (uint32_t i = 0; i < std::get<4>(var.second).size(); i++)
            {
                segments["LockFlag"] = std::get<4>(var.second)[i].first;
                segments["SegmentLength"] = std::get<4>(var.second)[i].second;
                myarray.push_back(segments);
            }

            returnJson["SegmentFlags"] = myarray;

            res.jsonValue["Record"] = returnJson;
            res.end();
            return;
        }
    }
}
void handleRelaseAllApi(const crow::Request &req, crow::Response &res)
{

    std::string sessionid = req.session->uniqueId;
    crow::ibm_mc_lock::Lock::getInstance().releaseLock(sessionid);
    res.result(boost::beast::http::status::ok);
    res.end();
    return;
}

void handleReleaseLockAPI(const crow::Request &req, crow::Response &res,
                          std::vector<uint32_t> listTransactionIds)
{
    BMCWEB_LOG_DEBUG << listTransactionIds.size();
    BMCWEB_LOG_DEBUG << "Data is present";
    for (uint32_t i = 0; i < listTransactionIds.size(); i++)
    {
        BMCWEB_LOG_DEBUG << listTransactionIds[i];
    }

    std::string clientId = "hmc-id";
    std::string sessionId = req.session->uniqueId;

    // validate the request ids

    const std::vector<uint32_t> &p = listTransactionIds;
    auto varReleaselock = crow::ibm_mc_lock::Lock::getInstance().releaseLock(
        p, std::make_pair(clientId, sessionId));

    if (!varReleaselock.first)
    {
        // validation Failed
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    else
    {
        auto statusRelease =
            std::get<crow::ibm_mc_lock::rcrelaselock>(varReleaselock.second);
        if (statusRelease.first)
        {
            // The current hmc owns all the locks, so we already released
            // them
            res.result(boost::beast::http::status::ok);
            res.end();
            return;
        }

        else
        {
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

            for (uint32_t i = 0; i < std::get<4>(var.second).size(); i++)
            {
                segments["LockFlag"] = std::get<4>(var.second)[i].first;
                segments["SegmentLength"] = std::get<4>(var.second)[i].second;
                myArray.push_back(segments);
            }

            returnJson["SegmentFlags"] = myArray;
            res.jsonValue["Record"] = returnJson;
            res.end();
            return;
        }
    }
}

void handleGetLockListAPI(const crow::Request &req, crow::Response &res,
                          std::vector<std::string> listSessionIds)
{
    BMCWEB_LOG_DEBUG << listSessionIds.size();

    auto status =
        crow::ibm_mc_lock::Lock::getInstance().getLockList(listSessionIds);
    if (status.first)
    {
        res.result(boost::beast::http::status::ok);

        auto var = std::get<std::vector<std::pair<uint32_t, lockrequests>>>(
            status.second);

        nlohmann::json lockRecords = nlohmann::json::array();

        for (auto transactionId : var)
        {
            for (auto lockRecord : transactionId.second)
            {
                nlohmann::json returnJson, segments;
                nlohmann::json myArray = nlohmann::json::array();

                returnJson["TransactionID"] = transactionId.first;
                returnJson["SessionID"] = std::get<0>(lockRecord);
                returnJson["HMCID"] = std::get<1>(lockRecord);
                returnJson["LockType"] = std::get<2>(lockRecord);
                returnJson["ResourceID"] = std::get<3>(lockRecord);
                auto elements = std::get<4>(lockRecord);

                for (auto element : elements)
                {
                    segments["LockFlag"] = element.first;
                    segments["SegmentLength"] = element.second;
                    myArray.push_back(segments);
                }

                returnJson["SegmentFlags"] = myArray;
                lockRecords.push_back(returnJson);
            }
        }
        res.jsonValue["Records"] = lockRecords;
        res.end();
        return;
    }
}

mctp_eid_t readEID()
{
    mctp_eid_t mctpEid = defaultEIDValue;
    std::ifstream eidFile{eidPath};
    if (!eidFile.good())
    {
        BMCWEB_LOG_ERROR << "Could not open host EID file";
    }
    else
    {
        std::string eid;
        eidFile >> eid;
        if (!eid.empty())
        {
            mctpEid = static_cast<mctp_eid_t>(atoi(eid.c_str()));
        }
        else
        {
            BMCWEB_LOG_ERROR << "EID file was empty";
        }
    }
    return mctpEid;
}

void handleCsrRequest(const crow::Request &req, crow::Response &res,
                      const std::string &csrString)
{
    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>(res);

    crow::connections::systemBus->async_method_call(
        [asyncResp, req, csrString](const boost::system::error_code errorCode,
                                    const std::variant<std::string> &value) {
            if (errorCode)
            {
                BMCWEB_LOG_ERROR
                    << "Error in getting OperatingSystemState. ec : "
                    << errorCode;
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                CSRHandlerRunning = false;
                return;
            }
            const std::string *status = std::get_if<std::string>(&value);
            if ((*status != "xyz.openbmc_project.State.OperatingSystem.Status."
                            "OSStatus.BootComplete") &&
                (*status != "xyz.openbmc_project.State.OperatingSystem.Status."
                            "OSStatus.Standby"))
            {
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = "Invalid HOST State";
                CSRHandlerRunning = false;
                return;
            }

            std::error_code ec;
            if (!std::filesystem::is_directory("/var/lib/bmcweb", ec))
            {
                std::filesystem::create_directory("/var/lib/bmcweb", ec);
            }
            if (ec)
            {
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                BMCWEB_LOG_DEBUG << "handleCsrRequest: Failed to prepare "
                                    "certificate directory. ec : "
                                 << ec;
                CSRHandlerRunning = false;
                asyncResp->res.end();
                return;
            }
            // write CSR string to CSR file
            std::ofstream CSRFile;
            CSRFile.open(CSR_CERT_PATH);
            if (CSRFile.fail())
            {
                BMCWEB_LOG_DEBUG
                    << "Error while opening the CSR file for writing";
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                CSRHandlerRunning = false;
                asyncResp->res.end();
                return;
            }
            CSRFile << csrString;
            CSRFile.close();

            std::fstream certFile;
            std::filesystem::path clientCert = CLIENT_CERT_PATH;
            certFile.open(clientCert, std::ios::out);
            if (certFile.fail())
            {
                BMCWEB_LOG_DEBUG
                    << "Error while opening the client certificate file";
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                asyncResp->res.jsonValue["Description"] = internalServerError;
                CSRHandlerRunning = false;
                asyncResp->res.end();
                return;
            }
            certFile.close();

            uint32_t length = csrString.length();
            uint8_t mctpEid = readEID();

            crow::connections::systemBus->async_method_call(
                [asyncResp, length, mctpEid](const boost::system::error_code ec,
                                             const uint8_t &instanceId) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "GetInstanceId failed. ec : " << ec;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    pldm_fileio_file_type fileType =
                        PLDM_FILE_TYPE_CERT_SIGNING_REQUEST;
                    uint32_t fileHandle = 0;
                    auto wd = -1;
                    int fd = pldm_open();
                    if (fd < 0)
                    {
                        BMCWEB_LOG_DEBUG
                            << "handleCertificatePost: pldm_open() failed";
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    std::array<uint8_t,
                               sizeof(pldm_msg_hdr) + sizeof(fileType) +
                                   sizeof(fileHandle) + sizeof(uint64_t)>
                        requestMsg;

                    auto request =
                        reinterpret_cast<pldm_msg *>(requestMsg.data());

                    auto rc = encode_new_file_req(instanceId, fileType,
                                                  fileHandle, length, request);

                    if (rc != PLDM_SUCCESS)
                    {
                        BMCWEB_LOG_ERROR << "encode_new_file_req failed.rc = "
                                         << rc;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    uint8_t *responseMsg = nullptr;
                    size_t responseMsgSize{};

                    ResponseStatus status = ResponseStatus::success;

                    rc = pldm_send_recv(mctpEid, fd, requestMsg.data(),
                                        requestMsg.size(), &responseMsg,
                                        &responseMsgSize);
                    if (rc < 0)
                    {
                        BMCWEB_LOG_ERROR << "pldm_send_recv failed. rc = "
                                         << rc;
                        status = ResponseStatus::failure;
                        responseMsg = nullptr;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        return;
                    }

                    if (status == ResponseStatus::success)
                    {
                        uint8_t completionCode = 0;
                        auto response =
                            reinterpret_cast<pldm_msg *>(responseMsg);

                        auto decodeRC = decode_new_file_resp(
                            response, PLDM_NEW_FILE_RESP_BYTES,
                            &completionCode);
                        if (decodeRC < 0)
                        {
                            BMCWEB_LOG_ERROR
                                << "decode_new_file_resp failed. rc = "
                                << decodeRC;
                            status = ResponseStatus::failure;
                            asyncResp->res.result(boost::beast::http::status::
                                                      internal_server_error);
                            asyncResp->res.jsonValue["Description"] =
                                internalServerError;
                            CSRHandlerRunning = false;
                            return;
                        }
                        else
                        {
                            if (completionCode != PLDM_SUCCESS)
                            {
                                BMCWEB_LOG_ERROR << "Bad PLDM completion code "
                                                    "COMPLETION_CODE="
                                                 << completionCode;
                                status = ResponseStatus::failure;
                                asyncResp->res.result(
                                    boost::beast::http::status::
                                        internal_server_error);
                                asyncResp->res.jsonValue["Description"] =
                                    internalServerError;
                                CSRHandlerRunning = false;
                                return;
                            }
                        }
                    }

                    fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
                    if (fd < 0)
                    {
                        BMCWEB_LOG_ERROR << "Failed to create inotify watch.";
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    wd =
                        inotify_add_watch(fd, CLIENT_CERT_PATH, IN_CLOSE_WRITE);
                    if (wd < 0)
                    {
                        BMCWEB_LOG_ERROR << "Failed to watch ClientCert file.";
                        close(fd);
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    struct pollfd fds;
                    fds.fd = fd;
                    fds.events = POLLIN;

                    rc = poll(&fds, 1, pollTimeout * 1000);
                    if (rc < 0)
                    {
                        BMCWEB_LOG_ERROR << "Failed to add event.";
                        inotify_rm_watch(fd, wd);
                        close(fd);
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }
                    else if (rc == 0)
                    {
                        BMCWEB_LOG_ERROR << "Poll timed out  " << pollTimeout;
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    std::ifstream inFile;
                    inFile.open(CLIENT_CERT_PATH);
                    if (inFile.fail())
                    {
                        BMCWEB_LOG_DEBUG << "Error while opening the client "
                                            "certificate file for reading";
                        asyncResp->res.result(
                            boost::beast::http::status::internal_server_error);
                        asyncResp->res.jsonValue["Description"] =
                            internalServerError;
                        CSRHandlerRunning = false;
                        return;
                    }

                    std::stringstream strStream;
                    strStream << inFile.rdbuf();
                    std::string str = strStream.str();
                    inFile.close();

                    remove(CLIENT_CERT_PATH);
                    remove(CSR_CERT_PATH);
                    if (responseMsg)
                    {
                        free(responseMsg);
                    }
                    CSRHandlerRunning = false;
                    asyncResp->res.jsonValue["Certificate"] = str;
                },
                "xyz.openbmc_project.PLDM", "/xyz/openbmc_project/pldm",
                "xyz.openbmc_project.PLDM.Requester", "GetInstanceId", mctpEid);
        },
        "xyz.openbmc_project.State.Host", "/xyz/openbmc_project/state/host0",
        "org.freedesktop.DBus.Properties", "Get",
        "xyz.openbmc_project.State.OperatingSystem.Status",
        "OperatingSystemState");
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{
    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                res.jsonValue["@odata.type"] =
                    "#ibmServiceRoot.v1_0_0.ibmServiceRoot";
                res.jsonValue["@odata.id"] = "/ibm/v1/";
                res.jsonValue["Id"] = "IBM Rest RootService";
                res.jsonValue["Name"] = "IBM Service Root";
                res.jsonValue["ConfigFiles"] = {
                    {"@odata.id", "/ibm/v1/Host/ConfigFiles"}};
                res.jsonValue["LockService"] = {
                    {"@odata.id", "/ibm/v1/HMC/LockService"}};
                res.jsonValue["Certificate"] = {
                    {"@odata.id", "/ibm/v1/Host/Certificate"}};
                res.end();
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                handleConfigFileList(res);
            });

    BMCWEB_ROUTE(app,
                 "/ibm/v1/Host/ConfigFiles/Actions/FileCollection.DeleteAll")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res) {
                deleteConfigFiles(res);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("PUT"_method, "GET"_method, "DELETE"_method)(
            [](const crow::Request &req, crow::Response &res,
               const std::string &path) { handleFileUrl(req, res, path); });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Certificate")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            res.jsonValue["@odata.type"] = "#Certificate.v1_0_0.Certificate";
            res.jsonValue["@odata.id"] = "/ibm/v1/Host/Certificate";
            res.jsonValue["Id"] = "Certificate";
            res.jsonValue["Name"] = "Certificate";
            res.jsonValue["Actions"]["SignCSR"] = {
                {"target", "/ibm/v1/Host/Actions/SignCSR"}};
            res.jsonValue["root"] = {
                {"target", "/ibm/v1/Host/Certificate/root"}};
        });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Certificate/root")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            std::filesystem::path rootCert = ROOT_CERT_PATH;
            if (!std::filesystem::exists(rootCert))
            {
                BMCWEB_LOG_ERROR << "RootCert file does not exist";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }
            std::ifstream inFile;
            inFile.open(ROOT_CERT_PATH);
            if (inFile.fail())
            {
                BMCWEB_LOG_DEBUG << "Error while opening the root certificate "
                                    "file for reading";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            std::stringstream strStream;
            strStream << inFile.rdbuf();
            std::string certStr = strStream.str();
            inFile.close();
            res.jsonValue["Certificate"] = certStr;
            res.end();
        });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Actions/SignCSR")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "POST"_method)([](const crow::Request &req, crow::Response &res) {
            std::string csrString;
            if (!redfish::json_util::readJson(req, res, "CsrString", csrString))
            {
                res.result(boost::beast::http::status::bad_request);
                res.end();
                return;
            }
            if (CSRHandlerRunning)
            {
                BMCWEB_LOG_ERROR << "Handler already running";
                res.result(boost::beast::http::status::service_unavailable);
                res.jsonValue["Description"] = "Resource In Use";
                res.end();
                return;
            }
            CSRHandlerRunning = true;
            handleCsrRequest(req, res, csrString);
        });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                getLockServiceData(res);
            });

    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.AcquireLock")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res) {
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
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res) {
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
                    handleRelaseAllApi(req, res);
                }
                else
                {
                    // Type Validation has been failed
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
            });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/LockService/Actions/LockService.GetLockList")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res) {
                std::vector<std::string> listSessionIds;

                if (!redfish::json_util::readJson(req, res, "SessionIDs",
                                                  listSessionIds))
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                handleGetLockListAPI(req, res, listSessionIds);
            });

    std::error_code ec;
    if (!std::filesystem::is_directory("/var/lib/bmcweb", ec))
    {
        std::filesystem::create_directory("/var/lib/bmcweb", ec);
    }
    if (ec)
    {
        BMCWEB_LOG_ERROR << "handleCsrRequest: Failed to prepare certificate "
                            "directory. ec : "
                         << ec;
        return;
    }

    std::filesystem::path rootCert = ROOT_CERT_PATH;
    if (!std::filesystem::exists(rootCert))
    {
        std::fstream file;
        file.open(rootCert, std::ios::out);
        if (file.fail())
        {
            BMCWEB_LOG_ERROR << "Error while opening the root certificate file";
            return;
        }
        file.close();
    }
}

} // namespace ibm_mc
} // namespace crow
