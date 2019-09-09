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

using namespace std; // cs00

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
        .methods("PUT"_method)([](const crow::Request &req, crow::Response &res,
                                  const std::string &path) {
            std::string objectPath = "/ibm/v1/host/" + path;
            handleFileUrl(req, res, objectPath);
        });
}
} // namespace openbmc_ibm_mc
} // namespace crow
