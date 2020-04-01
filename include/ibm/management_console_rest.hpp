#pragma once
#include <app.h>
#include <libpldm/base.h>
#include <libpldm/file_io.h>
#include <libpldm/platform.h>
#include <libpldm/pldm.h>
#include <poll.h>
#include <sys/inotify.h>
#include <tinyxml2.h>
#include <unistd.h>

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
constexpr auto eidPath = "/usr/share/pldm/host_eid";
constexpr mctp_eid_t defaultEIDValue = 9;

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
    lockrequests lockrequeststructure;
    for (auto element : body)
    {
        std::string locktype;
        uint64_t resourceid;

        segmentflags seginfo;
        std::vector<nlohmann::json> segmentflags;

        if (!redfish::json_util::readJson(element, res, "LockType", locktype,
                                          "ResourceID", resourceid,
                                          "SegmentFlags", segmentflags))
        {
            BMCWEB_LOG_DEBUG << "Not a Valid JSON";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
        BMCWEB_LOG_DEBUG << locktype;
        BMCWEB_LOG_DEBUG << resourceid;

        BMCWEB_LOG_DEBUG << "Segment Flags are present";

        for (auto e : segmentflags)
        {
            std::string lockflags;
            uint32_t segmentlength;

            if (!redfish::json_util::readJson(e, res, "LockFlag", lockflags,
                                              "SegmentLength", segmentlength))
            {
                res.result(boost::beast::http::status::bad_request);
                res.end();
                return;
            }

            BMCWEB_LOG_DEBUG << "Lockflag : " << lockflags;
            BMCWEB_LOG_DEBUG << "SegmentLength : " << segmentlength;

            seginfo.push_back(std::make_pair(lockflags, segmentlength));
        }
        lockrequeststructure.push_back(make_tuple(
            req.session->uniqueId, "hmc-id", locktype, resourceid, seginfo));
    }

    // print lock request

    for (uint32_t i = 0; i < lockrequeststructure.size(); i++)
    {
        BMCWEB_LOG_DEBUG << std::get<0>(lockrequeststructure[i]);
        BMCWEB_LOG_DEBUG << std::get<1>(lockrequeststructure[i]);
        BMCWEB_LOG_DEBUG << std::get<2>(lockrequeststructure[i]);
        BMCWEB_LOG_DEBUG << std::get<3>(lockrequeststructure[i]);

        for (const auto &p : std::get<4>(lockrequeststructure[i]))
        {
            BMCWEB_LOG_DEBUG << p.first << ", " << p.second;
        }
    }

    const lockrequests &t = lockrequeststructure;

    auto var_acquirelock =
        crow::ibm_mc_lock::lock::getInstance().Acquirelock(t);

    if (var_acquirelock.first)
    {
        // Either validity failure of there is a conflict with itself
        auto validity_status =
            std::get<std::pair<bool, int>>(var_acquirelock.second);

        if ((!validity_status.first) && (validity_status.second == 0))
        {
            BMCWEB_LOG_DEBUG << "Not a Valid record";
            BMCWEB_LOG_DEBUG << "Bad json in request";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
        if (validity_status.first && (validity_status.second == 1))
        {
            BMCWEB_LOG_DEBUG << "There is a conflict within itself";
            res.result(boost::beast::http::status::bad_request);
            res.end();
            return;
        }
    }
    else
    {
        auto conflict_status =
            std::get<crow::ibm_mc_lock::rc>(var_acquirelock.second);
        if (!conflict_status.first)
        {
            BMCWEB_LOG_DEBUG << "There is no conflict with the locktable";
            res.result(boost::beast::http::status::ok);

            auto var = std::get<uint32_t>(conflict_status.second);
            nlohmann::json returnjson;
            returnjson["id"] = var;
            res.jsonValue["TransactionID"] = var;
            res.end();
            return;
        }
        else
        {
            BMCWEB_LOG_DEBUG << "There is a conflict with the lock table";
            res.result(boost::beast::http::status::conflict);
            auto var = std::get<std::pair<uint32_t, lockrequest>>(
                conflict_status.second);
            nlohmann::json returnjson, segments;
            nlohmann::json myarray = nlohmann::json::array();
            returnjson["TransactionID"] = var.first;
            returnjson["SessionID"] = std::get<0>(var.second);
            returnjson["HMCID"] = std::get<1>(var.second);
            returnjson["LockType"] = std::get<2>(var.second);
            returnjson["ResourceID"] = std::get<3>(var.second);

            for (uint32_t i = 0; i < std::get<4>(var.second).size(); i++)
            {
                segments["LockFlag"] = std::get<4>(var.second)[i].first;
                segments["SegmentLength"] = std::get<4>(var.second)[i].second;
                myarray.push_back(segments);
            }

            returnjson["SegmentFlags"] = myarray;

            res.jsonValue["Record"] = returnjson;
            res.end();
            return;
        }
    }
}
void handleRelaseAllApi(const crow::Request &req, crow::Response &res)
{

    std::string sessionid = req.session->uniqueId;
    crow::ibm_mc_lock::lock::getInstance().releaselock(sessionid);
    res.result(boost::beast::http::status::ok);
    res.end();
    return;
}

void handleReleaseLockAPI(const crow::Request &req, crow::Response &res,
                          std::vector<uint32_t> listtransactionIDs)
{
    BMCWEB_LOG_DEBUG << listtransactionIDs.size();
    BMCWEB_LOG_DEBUG << "Data is present";
    for (uint32_t i = 0; i < listtransactionIDs.size(); i++)
    {
        BMCWEB_LOG_DEBUG << listtransactionIDs[i];
    }

    std::string clientid = "hmc-id";
    std::string sessionid = req.session->uniqueId;

    // validate the request ids
    const std::vector<uint32_t> &p = listtransactionIDs;
    auto var_releaselock = crow::ibm_mc_lock::lock::getInstance().Releaselock(
        p, std::make_pair(clientid, sessionid));

    if (!var_releaselock.first)
    {
        // validation Failed
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    else
    {
        auto status_release =
            std::get<crow::ibm_mc_lock::rcrelaselock>(var_releaselock.second);
        if (status_release.first)
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

            auto var = status_release.second;
            nlohmann::json returnjson, segments;
            nlohmann::json myarray = nlohmann::json::array();
            returnjson["TransactionID"] = var.first;
            returnjson["SessionID"] = std::get<0>(var.second);
            returnjson["HMCID"] = std::get<1>(var.second);
            returnjson["LockType"] = std::get<2>(var.second);
            returnjson["ResourceID"] = std::get<3>(var.second);

            for (uint32_t i = 0; i < std::get<4>(var.second).size(); i++)
            {
                segments["LockFlag"] = std::get<4>(var.second)[i].first;
                segments["SegmentLength"] = std::get<4>(var.second)[i].second;
                myarray.push_back(segments);
            }

            returnjson["SegmentFlags"] = myarray;
            res.jsonValue["Record"] = returnjson;
            res.end();
            return;
        }
    }
}

void handleGetLockListAPI(const crow::Request &req, crow::Response &res,
                          std::vector<std::string> listSessionIDs)
{
    BMCWEB_LOG_DEBUG << listSessionIDs.size();

    auto status =
        crow::ibm_mc_lock::lock::getInstance().getlocklist(listSessionIDs);
    if (status.first)
    {
        res.result(boost::beast::http::status::ok);

        auto var = std::get<std::vector<std::pair<uint32_t, lockrequests>>>(
            status.second);

        nlohmann::json lockrecords = nlohmann::json::array();

        for (auto transactionid : var)
        {
            for (auto lockrecord : transactionid.second)
            {
                nlohmann::json returnjson, segments;
                nlohmann::json myarray = nlohmann::json::array();

                returnjson["TransactionID"] = transactionid.first;
                returnjson["SessionID"] = std::get<0>(lockrecord);
                returnjson["HMCID"] = std::get<1>(lockrecord);
                returnjson["LockType"] = std::get<2>(lockrecord);
                returnjson["ResourceID"] = std::get<3>(lockrecord);
                auto elements = std::get<4>(lockrecord);

                for (auto element : elements)
                {
                    segments["LockFlag"] = element.first;
                    segments["SegmentLength"] = element.second;
                    myarray.push_back(segments);
                }

                returnjson["SegmentFlags"] = myarray;
                lockrecords.push_back(returnjson);
            }
        }
        res.jsonValue["Records"] = lockrecords;
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
    std::error_code ec;
    if (!std::filesystem::is_directory("/var/lib/bmcweb"))

    {
        std::filesystem::create_directory("/var/lib/bmcweb", ec);
    }

    // write CSR string to CSR file
    std::ofstream CSRFile, certFile;
    CSRFile.open("/var/lib/bmcweb/CSR");
    CSRFile << csrString;
    CSRFile.close();

    certFile.open("/var/lib/bmcweb/ClientCert");
    certFile.close();

    uint32_t length = csrString.length();

    mctp_eid_t mctpEid = readEID();

    std::shared_ptr<bmcweb::AsyncResp> asyncResp =
        std::make_shared<bmcweb::AsyncResp>(res);
    crow::connections::systemBus->async_method_call(
        [asyncResp, length, mctpEid, &res](const boost::system::error_code ec,
                                           std::variant<uint8_t> value) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << "GetInstanceId failed. ec : " << ec;
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }
            auto instanceId = std::get<uint8_t>(value);
            uint16_t fileType = 4; // PLDM_FILE_TYPE_CERT_SIGNING_REQUEST
            uint32_t fileHandle = 0;
            auto wd = -1;
            int fd = pldm_open();
            if (-1 == fd)
            {
                BMCWEB_LOG_DEBUG << "handleCertificatePost: pldm_open() failed";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            std::array<uint8_t, sizeof(pldm_msg_hdr) + sizeof(fileType) +
                                    sizeof(fileHandle) + sizeof(uint64_t)>
                requestMsg;

            auto request = reinterpret_cast<pldm_msg *>(requestMsg.data());

            auto rc = encode_new_file_req(instanceId, fileType, fileHandle,
                                          length, request);

            if (rc != PLDM_SUCCESS)
            {
                BMCWEB_LOG_ERROR << "encode_new_file_req failed.rc = " << rc;
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            rc = pldm_send(mctpEid, fd, requestMsg.data(), requestMsg.size());
            if (rc < 0)
            {
                BMCWEB_LOG_ERROR << "pldm_send failed. rc = " << rc;
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
            if (fd < 0)
            {
                BMCWEB_LOG_ERROR << "Failed to create inotify watch.";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            constexpr auto CLIENT_CERT_PATH = "/var/lib/bmcweb/ClientCert";
            wd = inotify_add_watch(fd, CLIENT_CERT_PATH, IN_MODIFY);
            if (wd < 0)
            {
                BMCWEB_LOG_ERROR << "Failed to watch ClientCert file.";
                close(fd);
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            struct pollfd fds;
            fds.fd = fd;
            fds.events = POLLIN;

            constexpr auto pollTimeout = 30;
            rc = poll(&fds, 1, pollTimeout * 1000);
            if (rc < 0)
            {
                BMCWEB_LOG_ERROR << "Failed to add event.";
                inotify_rm_watch(fd, wd);
                close(fd);
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }
            else if (rc == 0)
            {
                BMCWEB_LOG_ERROR << "Poll timed out  " << pollTimeout;
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }

            std::ifstream inFile;
            inFile.open("/var/lib/bmcweb/ClientCert");

            std::stringstream strStream;
            strStream << inFile.rdbuf(); // read the file
            std::string str = strStream.str();
            inFile.close();
            res.jsonValue["Certificate"] = str;

            remove("/var/lib/bmcweb/ClientCert");
            remove("/var/lib/bmcweb/CSR");
            res.end();
        },
        "xyz.openbmc_project.PLDM", "/xyz/openbmc_project/pldm",
        "xyz.openbmc_project.PLDM.Requester", "GetInstanceId", mctpEid);
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
            handleCsrRequest(req, res, csrString);
        });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/Certificate/root")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            std::filesystem::path rootCert = "/var/lib/bmcweb/RootCert";
            if (!std::filesystem::exists(rootCert))
            {
                BMCWEB_LOG_ERROR << "RootCert file does not exist";
                res.result(boost::beast::http::status::internal_server_error);
                res.jsonValue["Description"] = internalServerError;
                return;
            }
            std::ifstream inFile;
            inFile.open("/var/lib/bmcweb/RootCert");
            std::stringstream strStream;
            strStream << inFile.rdbuf();
            std::string certStr = strStream.str();
            inFile.close();
            res.jsonValue["Certificate"] = certStr;
            res.end();
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
                std::vector<uint32_t> listtransactionIDs;

                if (!redfish::json_util::readJson(req, res, "Type", type,
                                                  "TransactionIDs",
                                                  listtransactionIDs))

                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }

                if (type == "Transaction")
                {
                    handleReleaseLockAPI(req, res, listtransactionIDs);
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
                std::vector<std::string> listSessionIDs;
                if (!redfish::json_util::readJson(req, res, "SessionIDs",
                                                  listSessionIDs))
                {
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
                handleGetLockListAPI(req, res, listSessionIDs);
            });

    FILE *fp = fopen("/var/lib/bmcweb/RootCert", "w");
    if (fp == NULL)
    {
        return;
    }
    fclose(fp);
}

} // namespace ibm_mc
} // namespace crow
