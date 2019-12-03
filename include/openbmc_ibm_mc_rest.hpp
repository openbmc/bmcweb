#pragma once
#include <app.h>
#include <tinyxml2.h>

#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <nlohmann/json.hpp>
#include <regex>
#include <sdbusplus/message/types.hpp>
#include <utils/json_utils.hpp>

using namespace std; // cs00
using stype = std::string;
using segmentflags = std::vector<std::pair<std::string, uint32_t>>;
using lockrecord = std::tuple<stype, stype, stype, uint64_t, segmentflags>;
using lockrequest = std::vector<lockrecord>;
using rc = std::pair<bool, std::variant<std::vector<uint32_t>, lockrecord>>;
using rcrelaselock = std::pair<bool, lockrecord>;
using rcgetlocklist =
    std::pair<bool, std::variant<std::string, std::vector<lockrecord>>>;

namespace crow
{
namespace openbmc_ibm_mc
{

const std::string methodNotAllowedDesc = "Method not allowed";
const std::string methodNotAllowedMsg = "405 Method Not Allowed";
const std::string resourceNotFoundDesc = "Resource path or object not found";
const std::string resourceNotFoundMsg = "404 Not Found";
const std::string contentNotAcceptableDesc =
    "The request content is not allowed";
const std::string contentNotAcceptableMsg = "406 Not Acceptable";

class lock
{
    uint32_t rid;
    std::map<uint32_t, lockrecord> locktable;

  public:
    bool isvalidlockrecord(lockrecord *);
    bool isconflictrequest(lockrequest *);
    bool isconflictrecord(lockrecord *, lockrecord *);
    rc isconflictwithtable(lockrequest *);
    rcrelaselock isitmylock(std::vector<uint32_t> *, std::string);
    bool validaterids(std::vector<uint32_t> *);
    void releaselock(std::vector<uint32_t> *);
    rcgetlocklist getlocklist(std::string);
    bool checkbyte(uint64_t, uint64_t, uint32_t);
    void printmymap();

    uint32_t getmyrequestid();

  public:
    lock()
    {
        rid = 0;
    }

    // friend class locktest;
    template <typename... Middlewares>
    friend void requestRoutes(Crow<Middlewares...> &app);

} lockobject;

rcgetlocklist lock::getlocklist(std::string sessionid)
{

    // validate the session id
    std::vector<lockrecord> locklist;

    auto it = locktable.begin();

    while (it != locktable.end())
    {
        // Check if session id of this entry matches with session id given
        if (std::get<0>(it->second) == sessionid)
        {
            BMCWEB_LOG_DEBUG << "Session id is found in the locktable";

            // Push the whole lock record into a vector for returning the json
            locklist.push_back(it->second);
        }
        // Go to next entry in map
        it++;
    }
    if (locklist.size() == 0)
    {
        // The session id is not found in the lock table
        // return a validation failure
        return std::make_pair(false, sessionid);
    }
    else
    {
        // we found at least one entry with the given session id
        // return the json list of lock records pertaining to the
        // given session id
        return std::make_pair(true, locklist);
    }

    return std::make_pair(true, sessionid);
}

void lock::releaselock(std::vector<uint32_t> *refrids)
{
    for (uint32_t i = 0; i < refrids->size(); i++)
    {
        locktable.erase(refrids->at(i));
    }
}

rcrelaselock lock::isitmylock(std::vector<uint32_t> *refrids,
                              std::string clientid)
{
    for (uint32_t i = 0; i < refrids->size(); i++)
    {
        std::string expectedclientid = std::get<1>(locktable[refrids->at(i)]);

        if (expectedclientid == clientid)
        {
            // It is owned by the currently request hmc
            BMCWEB_LOG_DEBUG << "Lock is owned  by the current hmc";
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Lock is not owned by the current hmc";
            return std::make_pair(false, locktable[refrids->at(i)]);
        }
    }
    return std::make_pair(true, lockrecord());
}

bool lock::validaterids(std::vector<uint32_t> *refrids)
{
    for (uint32_t i = 0; i < refrids->size(); i++)
    {
        auto search = locktable.find(refrids->at(i));

        if (search != locktable.end())
        {
            BMCWEB_LOG_DEBUG << "Valid Request id";
            //  continue for the next rid
        }
        else
        {
            BMCWEB_LOG_DEBUG << "Atleast 1 inValid Request id";
            return false;
        }
    }
    return true;
}

void lock::printmymap()
{

    BMCWEB_LOG_DEBUG << "Printing the locktable";

    for (auto it = locktable.begin(); it != locktable.end(); it++)
    {
        BMCWEB_LOG_DEBUG << it->first;
        BMCWEB_LOG_DEBUG << std::get<0>(it->second);
        BMCWEB_LOG_DEBUG << std::get<1>(it->second);
        BMCWEB_LOG_DEBUG << std::get<2>(it->second);
        BMCWEB_LOG_DEBUG << std::get<3>(it->second);

        for (const auto &p : std::get<4>(it->second))
        {
            BMCWEB_LOG_DEBUG << p.first << ", " << p.second;
        }
    }
}

bool lock::isvalidlockrecord(lockrecord *reflockrecord)
{

    // validate the locktype

    if (!((boost::iequals(std::get<2>(*reflockrecord), "read") ||
           (boost::iequals(std::get<2>(*reflockrecord), "write")))))
    {
        BMCWEB_LOG_DEBUG << "Locktype : " << std::get<2>(*reflockrecord);
        return false;
    }

    // validate the resource id

    // validate the lockflags & segment length

    for (const auto &p : std::get<4>(*reflockrecord))
    {

        // validate the lock flags
        if (!((boost::iequals(p.first, "locksame") ||
               (boost::iequals(p.first, "lockall")) ||
               (boost::iequals(p.first, "dontlock")))))
        {
            BMCWEB_LOG_DEBUG << p.first;
            return false;
        }

        // validate the segment length
        if (p.second < 1 || p.second > 4)
        {
            BMCWEB_LOG_DEBUG << p.second;
            return false;
        }
    }

    // validate the segment length
    return true;
}

rc lock::isconflictwithtable(lockrequest *reflockrequeststructure)
{

    uint32_t rid;
    std::vector<uint32_t> vrid;

    if (locktable.empty())
    {
        for (uint32_t i = 0; i < reflockrequeststructure->size(); i++)
        {
            // Lock table is empty, so we are safe to add the lockrecords
            // as there will be no conflict
            BMCWEB_LOG_DEBUG
                << "Lock table is empty, so adding the lockrecords";
            rid = getmyrequestid();
            locktable[rid] = reflockrequeststructure->at(i);
            vrid.push_back(rid);
        }
        return std::make_pair(false, vrid);
    }

    else
    {
        BMCWEB_LOG_DEBUG
            << "Lock table is not empty, check for conflict with lock table";
        // Lock table is not empty, compare the lockrequest entries with
        // the entries in the lock table

        for (uint32_t i = 0; i < reflockrequeststructure->size(); i++)
        {
            for (auto it = locktable.begin(); it != locktable.end(); it++)
            {
                bool status = isconflictrecord(&reflockrequeststructure->at(i),
                                               &it->second);
                if (status)
                {
                    return std::make_pair(true, it->second);
                }
                else
                {
                    // No conflict , we can proceed to another record
                }
            }
        }

        // Reached here, so no conflict with the locktable, so we are safe to
        // add the request records into the lock table

        for (uint32_t i = 0; i < reflockrequeststructure->size(); i++)
        {
            // Lock table is empty, so we are safe to add the lockrecords
            // as there will be no conflict
            BMCWEB_LOG_DEBUG << " Adding elements into lock table";
            rid = getmyrequestid();
            locktable[rid] = reflockrequeststructure->at(i);
            vrid.push_back(rid);
        }
    }
    return std::make_pair(false, vrid);
}

bool lock::isconflictrequest(lockrequest *reflockrequeststructure)
{
    // check for all the locks coming in as a part of single request
    // return conflict if any two lock requests are conflicting

    if (reflockrequeststructure->size() == 1)
    {
        BMCWEB_LOG_DEBUG << "Only single lock request, so there is no conflict";
        // This means , we have only one lock request in the current
        // request , so no conflict within the request
        return false;
    }
    else
    {
        BMCWEB_LOG_DEBUG
            << "There are multiple lock requests coming in a single request";

        // There are multiple requests a part of one request
        for (uint32_t i = 0; i < reflockrequeststructure->size(); i++)
        {
            for (uint32_t j = i + 1; j < reflockrequeststructure->size(); j++)
            {
                bool status = isconflictrecord(&reflockrequeststructure->at(i),
                                               &reflockrequeststructure->at(j));

                if (status)
                {
                    return true;
                }
            }
        }
    }
    return false;
}

bool lock::checkbyte(uint64_t resourceid1, uint64_t resourceid2, uint32_t j)
{
    uint8_t *p = reinterpret_cast<uint8_t *>(&resourceid1);
    uint8_t *q = reinterpret_cast<uint8_t *>(&resourceid2);

    // uint8_t result[8];
    // for(uint32_t i = 0; i < j; i++)
    // {
    BMCWEB_LOG_DEBUG << "Comparing bytes " << std::to_string(p[j]) << ","
                     << std::to_string(q[j]);
    if (p[j] != q[j])
    {
        return false;
    }

    else
    {
        return true;
    }

    //}
    return true;
}

bool lock::isconflictrecord(lockrecord *reflockrecord1,
                            lockrecord *reflockrecord2)
{
    // No conflict if both are read locks

    if (boost::iequals(std::get<2>(*reflockrecord1), "read") &&
        boost::iequals(std::get<2>(*reflockrecord2), "read"))
    {
        BMCWEB_LOG_DEBUG << "Both are read locks, no conflict";
        return false;
    }

    uint32_t i = 0;
    for (const auto &p : std::get<4>(*reflockrecord1))
    {

        // return conflict when any of them is try to lock all resources under
        // the current resource level.
        if (boost::iequals(p.first, "lockall") ||
            boost::iequals(std::get<4>(*reflockrecord2)[i].first, "lockall"))
        {
            BMCWEB_LOG_DEBUG
                << "Either of the Comparing locks are trying to lock all "
                   "resources under the current resource level";
            return true;
        }

        // determine if there is a lock-all-with-same-segment-size.
        // If the current segment sizes are the same,then we should fail.

        if ((boost::iequals(p.first, "locksame") ||
             boost::iequals(std::get<4>(*reflockrecord2)[i].first,
                            "locksame")) &&
            (p.second == std::get<4>(*reflockrecord2)[i].second))
        {
            return true;
        }

        // if segment lengths are not the same, it means two different locks
        // So no conflict
        if (p.second != std::get<4>(*reflockrecord2)[i].second)
        {
            BMCWEB_LOG_DEBUG << "Segment lengths are not same";
            BMCWEB_LOG_DEBUG << "Segment 1 length : " << p.second;
            BMCWEB_LOG_DEBUG << "Segment 2 length : "
                             << std::get<4>(*reflockrecord2)[i].second;
            return false;
        }

        // compare segment data

        for (uint32_t i = 0; i < p.second; i++)
        {
            // if the segment data is different , then the locks is on a
            // different resource So no conflict between the lock records
            if (!(checkbyte(std::get<3>(*reflockrecord1),
                            std::get<3>(*reflockrecord2), i)))
            {
                return false;
            }
        }

        ++i;
    }

    return false;
}

uint32_t lock::getmyrequestid()
{
    ++rid;
    return rid;
}

void setErrorResponse(crow::Response &res, boost::beast::http::status result,
                      const std::string &desc, const std::string &msg)
{
    res.result(result);
    res.jsonValue = {{"data", {{"description", desc}}},
                     {"message", msg},
                     {"status", "error"}};
}

bool createSaveAreaPath(crow::Response &res)
{

    // The path /var/lib/obmc will be created by initrdscripts
    // Create the directories for the save-area files, when we get
    // first file upload request
    try
    {
        if (!std::filesystem::is_directory("/var/lib/obmc/bmc-console-mgmt"))
        {
            std::filesystem::create_directory("/var/lib/obmc/bmc-console-mgmt");
        }
        if (!std::filesystem::is_directory(
                "/var/lib/obmc/bmc-console-mgmt/save-area"))
        {
            std::filesystem::create_directory(
                "/var/lib/obmc/bmc-console-mgmt/save-area");
        }
    }
    catch (const std::filesystem::filesystem_error &e)
    {
        setErrorResponse(res, boost::beast::http::status::internal_server_error,
                         "Failed to create save-area directory",
                         "Internal Error");
        BMCWEB_LOG_DEBUG << "handleIbmPost: Failed to prepare save-area dir";
        return false;
    }
    return true;
}
void handleFilePut(const crow::Request &req, crow::Response &res,
                   const std::string &objectPath,
                   const std::string &destProperty)
{
    // Check the content-type of the request
    std::string_view contentType = req.getHeaderValue("content-type");
    if (boost::starts_with(contentType, "multipart/form-data"))
    {
        BMCWEB_LOG_DEBUG
            << "This is multipart/form-data. Invalid content for PUT";

        setErrorResponse(res, boost::beast::http::status::not_acceptable,
                         contentNotAcceptableDesc, contentNotAcceptableMsg);
        return;
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Not a multipart/form-data. Continue..";
    }
    std::size_t pos = objectPath.find("partitions/");
    if (pos != std::string::npos)
    {
        BMCWEB_LOG_DEBUG
            << "handleIbmPut: Request to create/update the save-area file";
        if (!createSaveAreaPath(res))
        {
            return;
        }
        // Extract the file name from the objectPath
        std::string fileId = objectPath.substr(pos + strlen("partitions/"));
        // Create the file
        std::ofstream file;
        std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
        file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        loc /= fileId;
        std::string data = std::move(req.body);
        try
        {
            BMCWEB_LOG_DEBUG << "Creating file " << loc;
            file.open(loc, std::ofstream::out);
            file << data;
            file.close();
            BMCWEB_LOG_DEBUG << "save-area file is created";
        }
        catch (std::ofstream::failure &e)
        {
            BMCWEB_LOG_DEBUG << "Error while opening the file for writing";
            return;
        }
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Bad URI";
        setErrorResponse(res, boost::beast::http::status::method_not_allowed,
                         resourceNotFoundDesc, resourceNotFoundMsg);
    }
    return;
}

void handleFileList(crow::Response &res, std::string &objectPath)
{
    BMCWEB_LOG_DEBUG << "HandleList of SaveArea files on Path: " << objectPath;

    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    if (!std::filesystem::exists(loc) || !std::filesystem::is_directory(loc))
    {
        BMCWEB_LOG_ERROR << loc << " Not found";
        setErrorResponse(res, boost::beast::http::status::not_found,
                         resourceNotFoundDesc, resourceNotFoundMsg);
        return;
    }

    for (const auto &file : std::filesystem::directory_iterator(loc))
    {
        std::filesystem::path pathObj(file.path());
        res.body().append(pathObj.filename().string() + "\n");
    }
}

void handleFileGet(crow::Response &res, std::string &objectPath,
                   std::string &destProperty)
{
    std::string basePath("/ibm/v1/host/partitions");

    if (boost::iequals(objectPath, basePath) ||
        boost::iequals(objectPath, basePath.append("/")))
    {
        handleFileList(res, objectPath);
        return;
    }
    else
    {
        BMCWEB_LOG_DEBUG << "HandleGet on SaveArea files on path: "
                         << objectPath;

        if (!boost::starts_with(objectPath, basePath))
        {
            BMCWEB_LOG_ERROR << "Invalid URI path: " << objectPath;
            setErrorResponse(res, boost::beast::http::status::not_found,
                             resourceNotFoundDesc, resourceNotFoundMsg);
            return;
        }

        std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area/");
        if (!std::filesystem::exists(loc) ||
            !std::filesystem::is_directory(loc))
        {
            BMCWEB_LOG_ERROR << loc << " Not found";
            setErrorResponse(res, boost::beast::http::status::not_found,
                             resourceNotFoundDesc, resourceNotFoundMsg);
            return;
        }

        std::string filename;
        std::filesystem::path pathObj(objectPath);
        if (!pathObj.has_filename() ||
            !boost::iequals(objectPath, basePath + pathObj.filename().string()))
        {
            BMCWEB_LOG_DEBUG << "File name not specified / Invalid file path";
            setErrorResponse(res, boost::beast::http::status::not_found,
                             resourceNotFoundDesc, resourceNotFoundMsg);
            return;
        }

        filename = pathObj.filename().string();
        std::string path(loc);
        path.append(filename);

        std::ifstream readfile(path);
        if (!readfile)
        {
            BMCWEB_LOG_ERROR << path << " Not found";
            setErrorResponse(res, boost::beast::http::status::not_found,
                             resourceNotFoundDesc, resourceNotFoundMsg);
            return;
        }

        std::string contentDispositionParam =
            "attachment; filename=\"" + filename + "\"";
        res.addHeader("Content-Disposition", contentDispositionParam);
        res.body() = {std::istreambuf_iterator<char>(readfile),
                      std::istreambuf_iterator<char>()};
    }
    return;
}

void handleFileDelete(const crow::Request &req, crow::Response &res,
                      const std::string &objectPath)
{
    BMCWEB_LOG_DEBUG << "HandleDelete of SaveArea files on Path: "
                     << objectPath;

    std::string basePath("/ibm/v1/host/partitions/");

    if (!boost::starts_with(objectPath, basePath))
    {
        BMCWEB_LOG_ERROR << "Invalid URI path: " << objectPath;
        setErrorResponse(res, boost::beast::http::status::not_found,
                         resourceNotFoundDesc, resourceNotFoundMsg);
        return;
    }

    std::string filename =
        objectPath.substr(basePath.length(), objectPath.length());

    std::string path("/var/lib/obmc/bmc-console-mgmt/save-area/");
    std::string filePath(path + filename);

    BMCWEB_LOG_DEBUG << "Removing the file : " << filePath << "\n";

    std::ifstream file_open(filePath.c_str());
    if (static_cast<bool>(file_open))
        if (remove(filePath.c_str()) == 0)
        {
            BMCWEB_LOG_DEBUG << "File removed!\n";
            res.jsonValue = {{"message", "200 OK"},
                             {"status", "ok"},
                             {"data", "File Removed"}};
        }
        else
        {
            BMCWEB_LOG_ERROR << "File not removed!\n";
            setErrorResponse(
                res, boost::beast::http::status::internal_server_error,
                "File name not found", "500 Internal Server Error");
        }
    else
    {
        BMCWEB_LOG_ERROR << "File not found!\n";
        setErrorResponse(res, boost::beast::http::status::not_found,
                         resourceNotFoundDesc, resourceNotFoundMsg);
    }
    return;
}

inline void handleFileUrl(const crow::Request &req, crow::Response &res,
                          std::string &objectPath)
{
    std::string destProperty = "";
    if (req.method() == "PUT"_method)
    {
        handleFilePut(req, res, objectPath, destProperty);
        res.end();
        return;
    }

    if (req.method() == "GET"_method)
    {
        handleFileGet(res, objectPath, destProperty);
        res.end();
        return;
    }

    if (req.method() == "DELETE"_method)
    {
        handleFileDelete(req, res, objectPath);
        res.end();
        return;
    }

    setErrorResponse(res, boost::beast::http::status::method_not_allowed,
                     methodNotAllowedDesc, methodNotAllowedMsg);
    res.end();
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{

    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/host/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("PUT"_method, "GET"_method, "DELETE"_method)(
            [](const crow::Request &req, crow::Response &res,
               const std::string &path) {
                std::string objectPath = "/ibm/v1/host/" + path;
                handleFileUrl(req, res, objectPath);
            });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/locks/AcquireLock")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "POST"_method)([](const crow::Request &req, crow::Response &res) {
            std::vector<nlohmann::json> body;

            if (!redfish::json_util::readJson(req, res, "Request", body))
            {
                return;
            }
            BMCWEB_LOG_DEBUG << body.size();
            BMCWEB_LOG_DEBUG << "Data is present";

            lockrequest lockrequeststructure;
            for (auto element : body)
            {
                std::string locktype;
                uint64_t resourceid;

                segmentflags seginfo;
                std::vector<nlohmann::json> segmentflags;

                if (!redfish::json_util::readJson(
                        element, res, "LockType", locktype, "ResourceID",
                        resourceid, "SegmentFlags", segmentflags))
                {
                    return;
                }
                BMCWEB_LOG_DEBUG << locktype;
                BMCWEB_LOG_DEBUG << resourceid;

                BMCWEB_LOG_DEBUG << "Segment Flags are present";

                for (auto e : segmentflags)
                {
                    std::string lockflags;
                    uint32_t segmentlength;

                    if (!redfish::json_util::readJson(
                            e, res, "LockFlags", lockflags, "Segmentlength",
                            segmentlength))
                    {
                        return;
                    }

                    BMCWEB_LOG_DEBUG << "Lockflags : " << lockflags;
                    BMCWEB_LOG_DEBUG << "Segmentlength : " << segmentlength;

                    seginfo.push_back(std::make_pair(lockflags, segmentlength));
                }
                lockrequeststructure.push_back(make_tuple(req.session->uniqueId,
                                                          "hmc-id", locktype,
                                                          resourceid, seginfo));
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
                    // std::cout << std::get<0>(p) << ", " << std::get<1>(p) <<
                    // std::endl;
                }
            }

            // Validate the lock record

            for (uint32_t i = 0; i < lockrequeststructure.size(); i++)
            {
                bool status =
                    lockobject.isvalidlockrecord(&lockrequeststructure[i]);

                if (!status)
                {
                    BMCWEB_LOG_DEBUG << "Not a Valid record";
                    BMCWEB_LOG_DEBUG << "Bad json in request";
                    res.result(boost::beast::http::status::bad_request);
                    res.end();
                    return;
                }
            }

            // check for conflict record

            bool status = lockobject.isconflictrequest(&lockrequeststructure);

            if (status)
            {
                BMCWEB_LOG_DEBUG << "There is a conflict within itself";
                res.result(boost::beast::http::status::conflict);
                res.end();
                return;
            }
            else
            {
                BMCWEB_LOG_DEBUG
                    << "The request is not conflicting within itself";

                // Need to check for conflict with the locktable entries.

                auto status =
                    lockobject.isconflictwithtable(&lockrequeststructure);

                // print my map

                lockobject.printmymap();

                if (!status.first)
                {
                    BMCWEB_LOG_DEBUG
                        << "There is no conflict with the locktable";
                    res.result(boost::beast::http::status::ok);

                    nlohmann::json myarray = nlohmann::json::array();
                    auto var = std::get<std::vector<uint32_t>>(status.second);
                    for (uint32_t i = 0; i < var.size(); i++)
                    {
                        nlohmann::json returnjson;
                        returnjson["id"] = var[i];
                        myarray.push_back(returnjson);
                    }
                    res.jsonValue["Request"] = myarray;
                    res.end();
                    return;
                }
                else
                {
                    BMCWEB_LOG_DEBUG
                        << "There is a conflict with the lock table";
                    res.result(boost::beast::http::status::conflict);
                    auto var = std::get<lockrecord>(status.second);
                    nlohmann::json returnjson, segments;
                    nlohmann::json myarray = nlohmann::json::array();

                    returnjson["HMCID"] = std::get<1>(var);
                    returnjson["LockType"] = std::get<2>(var);
                    returnjson["ResourceID"] = std::get<3>(var);

                    for (uint32_t i = 0; i < std::get<4>(var).size(); i++)
                    {
                        segments["LockFlags"] = std::get<4>(var)[i].first;
                        segments["Segmentlength"] = std::get<4>(var)[i].second;
                        myarray.push_back(segments);
                    }

                    returnjson["SegmentFlags"] = myarray;

                    res.jsonValue["Record"] = returnjson;
                    res.end();
                    return;
                }
            }
        });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/locks/ReleaseLock")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "POST"_method)([](const crow::Request &req, crow::Response &res) {
            nlohmann::json body;

            if (!redfish::json_util::readJson(req, res, "Request", body))
            {
                return;
            }
            BMCWEB_LOG_DEBUG << body.size();
            BMCWEB_LOG_DEBUG << "Data is present";

            std::vector<uint32_t> requestids;

            for (auto e : body)
            {
                uint32_t requestid;
                if (!redfish::json_util::readJson(e, res, "id", requestid))
                {
                    return;
                }
                requestids.push_back(requestid);
            }

            for (uint32_t i = 0; i < requestids.size(); i++)
            {
                BMCWEB_LOG_DEBUG << requestids[i];
            }

            std::string clientid = "hmc-id";
            // validate the request ids

            bool status = lockobject.validaterids(&requestids);

            if (status)
            {
                // Validation of rids failed
                BMCWEB_LOG_DEBUG << "Not a Valid request id";
                res.result(boost::beast::http::status::bad_request);
                // res.jsonValue = {{"Record", {{"id","Atleast 1 Invalid id"}}},
                //                       {"message", "msg"},
                //                        {"status", "error"}};
                res.end();
                return;
            }
            else
            {
                // Validation passed, check if all the locks are owned by the
                // requesting HMC
                auto status = lockobject.isitmylock(&requestids, clientid);
                if (status.first)
                {
                    // The current hmc owns all the locks, so we can release
                    // them
                    lockobject.releaselock(&requestids);
                    res.result(boost::beast::http::status::ok);
                    res.end();
                    return;
                }
                else
                {
                    // valid rid, but the current hmc does not own all the locks
                    BMCWEB_LOG_DEBUG
                        << "Current HMC does not own all the locks";
                    res.result(boost::beast::http::status::unauthorized);

                    auto var = status.second;
                    nlohmann::json returnjson, segments;
                    nlohmann::json myarray = nlohmann::json::array();

                    returnjson["HMCID"] = std::get<1>(var);
                    returnjson["LockType"] = std::get<2>(var);
                    returnjson["ResourceID"] = std::get<3>(var);

                    for (uint32_t i = 0; i < std::get<4>(var).size(); i++)
                    {
                        segments["LockFlags"] = std::get<4>(var)[i].first;
                        segments["Segmentlength"] = std::get<4>(var)[i].second;
                        myarray.push_back(segments);
                    }

                    returnjson["SegmentFlags"] = myarray;
                    res.jsonValue["Record"] = returnjson;
                    res.end();
                    return;
                }
            }
        });
    BMCWEB_ROUTE(app, "/ibm/v1/HMC/locks/GetLockList")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res) {
                nlohmann::json body;

                if (!redfish::json_util::readJson(req, res, "Request", body))
                {
                    return;
                }
                BMCWEB_LOG_DEBUG << body.size();
                BMCWEB_LOG_DEBUG << "Data is present";
                std::string sessionid;

                if (!redfish::json_util::readJson(body, res, "id", sessionid))
                {
                    return;
                }

                auto status = lockobject.getlocklist(sessionid);
                if (status.first)
                {
                    res.result(boost::beast::http::status::ok);

                    auto var = std::get<std::vector<lockrecord>>(status.second);
                    nlohmann::json myarray2 = nlohmann::json::array();
                    for (uint32_t i = 0; i < var.size(); i++)
                    {
                        nlohmann::json returnjson, segments;
                        nlohmann::json myarray = nlohmann::json::array();

                        returnjson["HMCID"] = std::get<1>(var[i]);
                        returnjson["LockType"] = std::get<2>(var[i]);
                        returnjson["ResourceID"] = std::get<3>(var[i]);
                        auto element = std::get<4>(var[i]);
                        for (uint32_t j = 0; j < element.size(); j++)
                        {
                            segments["LockFlags"] = element[j].first;
                            segments["Segmentlength"] = element[j].second;
                            myarray.push_back(segments);
                        }

                        returnjson["SegmentFlags"] = myarray;
                        myarray2.push_back(returnjson);
                    }

                    res.jsonValue["Record"] = myarray2;
                    res.end();
                    return;
                }
                else
                {
                    res.result(boost::beast::http::status::bad_request);
                    // res.jsonValue = {{"Record", {{"id",status.second}}}};
                    res.end();
                    return;
                }
            });
}
} // namespace openbmc_ibm_mc
} // namespace crow
