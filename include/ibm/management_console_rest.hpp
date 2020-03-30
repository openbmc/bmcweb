#pragma once
#include <app.h>
#include <tinyxml2.h>

#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sdbusplus/message/types.hpp>
#include <mimetic/mimetic.h>

#define MAX_SAVE_AREA_FILESIZE 200000

namespace crow
{
namespace ibm_mc
{
#include <nlohmann/json.hpp>
using namespace mimetic;
using json = nlohmann::json;

constexpr const char *methodNotAllowedMsg = "Method Not Allowed";
constexpr const char *resourceNotFoundMsg = "Resource Not Found";
constexpr const char *contentNotAcceptableMsg = "Content Not Acceptable";
constexpr const char *internalServerError = "Internal Server Error";

bool createSaveAreaPath(crow::Response &res)
{
    // The path /var/lib/obmc will be created by initrdscripts
    // Create the directories for the save-area files, when we get
    // first file upload request
    std::error_code ec;
    if (!std::filesystem::is_directory("/var/lib/obmc/bmc-console-mgmt"))
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
            "/var/lib/obmc/bmc-console-mgmt/save-area"))
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

bool createMultipartFiles(MimeEntity *pMe)
{
    Header &h = pMe->header();

    // create the file
    std::string fileId =
        h.contentDisposition().ContentDisposition::param("name");

    if (!fileId.empty())
    {
        std::ofstream sa_file;
        std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
        sa_file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
        loc /= fileId;
        try
        {
            sa_file.open(loc, std::ofstream::out);
            sa_file << pMe->body();
            sa_file.close();
            BMCWEB_LOG_DEBUG << "Created save-area file: " << loc;
        }
        catch (std::ofstream::failure &e)
        {
            BMCWEB_LOG_DEBUG << "Error while opening the save area file for writing: " << fileId;
            return false;
        }
    }
    // Iterate over the data to create next files
    MimeEntityList &parts = pMe->body().parts(); // list of sub entities obj
    MimeEntityList::iterator mbit = parts.begin(), meit = parts.end();
    for (; mbit != meit; ++mbit)
    {
        if (createMultipartFiles(*mbit)) {
            continue;
        } else {
            return false;
        }
    }
    return true;
}

void handleFilePost(const crow::Request &req, crow::Response &res,
                   const std::string &fileID)
{
    BMCWEB_LOG_DEBUG
        << "handleIbmPost: Request for multipart upload of save-area files";
    if (!createSaveAreaPath(res))
    {
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
        return;
    }

    // Using the req.body and the req header, prepare the mimetic parsable data
    std::string data = std::move(req.body);

    json j_data = json::parse(data, nullptr, false);
    if ( j_data.is_discarded() ) {
        BMCWEB_LOG_ERROR << "Ill-formed JSON";
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = internalServerError;
        return;
    }

    data = j_data["filedata"];

    // create a temporary file, which will become the input to the mimetic
    // constructor
    std::ofstream out("tmp.txt");
    out << data;
    out.close();
    std::ifstream fin("tmp.txt");

    // Get input stream and end of stream iterators
    std::istreambuf_iterator<char> fin_it(fin);
    std::istreambuf_iterator<char> eos;

    MimeEntity me(fin_it, eos);
    BMCWEB_LOG_DEBUG << "Creating files from a multipart requesti data of size: " << me.size();

    if (createMultipartFiles(&me)) {
        res.jsonValue["Description"] = "save-area files created";
    } else {
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["Description"] = internalServerError;
        BMCWEB_LOG_DEBUG
            << "handleFilePost: Failed to create save-area files";
    }
    fin.close();    
    return;
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

    return;
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
    if (req.method() == "POST"_method)
    {   
        if (fileID.compare("ConfigFiles") == 0 || fileID.compare("ConfigFiles/") == 0) {
            handleFilePost(req, res, fileID);
        } else {
            BMCWEB_LOG_DEBUG << "Invalid Path";
            res.result(boost::beast::http::status::not_found);
            res.jsonValue["Description"] = resourceNotFoundMsg;
        }

        res.end();
        return;
    }
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

    BMCWEB_ROUTE(app, "/ibm/v1/Host/ConfigFiles/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("PUT"_method, "GET"_method, "DELETE"_method)(
            [](const crow::Request &req, crow::Response &res,
               const std::string &path) { handleFileUrl(req, res, path); });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method)(
            [](const crow::Request &req, crow::Response &res,
               const std::string &path) { handleFileUrl(req, res, path); });
}

} // namespace ibm_mc
} // namespace crow
