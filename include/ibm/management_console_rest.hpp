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

#define MAX_SAVE_AREA_FILESIZE 200000

namespace crow
{
namespace ibm_mc
{
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

        res.result(boost::beast::http::status::not_acceptable);
        res.jsonValue["Description"] = contentNotAcceptableMsg;
        return;
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Not a multipart/form-data. Continue..";
    }
    std::size_t pos = objectPath.find("ConfigFiles/");
    if (pos != std::string::npos)
    {
        BMCWEB_LOG_DEBUG
            << "handleIbmPut: Request to create/update the save-area file";
        if (!createSaveAreaPath(res))
        {
            res.result(boost::beast::http::status::not_found);
            res.jsonValue["Description"] = resourceNotFoundMsg;
            return;
        }
        // Extract the file name from the objectPath
        std::string fileId = objectPath.substr(pos + strlen("ConfigFiles/"));
        // Create the file
        std::ofstream file;
        std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
        loc /= fileId;

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
    else
    {
        BMCWEB_LOG_DEBUG << "Bad URI";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["Description"] = resourceNotFoundMsg;
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
    res.result(boost::beast::http::status::method_not_allowed);
    res.jsonValue["Description"] = methodNotAllowedMsg;
    res.end();
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{

    // allowed only for admin
    BMCWEB_ROUTE(app, "/ibm/v1/")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods(
            "GET"_method)([](const crow::Request &req, crow::Response &res) {
            res.jsonValue["@odata.type"] = "#ibmServiceRoot.v1_0_0.ibmServiceRoot";
            res.jsonValue["@odata.id"] = "/ibm/v1/";
            res.jsonValue["Id"] = "IBM Rest RootService";
            res.jsonValue["Name"] = "IBM Service Root";
            res.jsonValue["ConfigFiles"] = {
                {"@odata.id", "/ibm/v1/Host/ConfigFiles"}};
            res.jsonValue["LockService"] = {
                {"@odata.id", "/ibm/v1/HMC/LockService"}};
            res.end();
        });

    BMCWEB_ROUTE(app, "/ibm/v1/Host/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("PUT"_method)([](const crow::Request &req, crow::Response &res,
                                  const std::string &path) {
            std::string objectPath = "/ibm/v1/Host/" + path;
            handleFileUrl(req, res, objectPath);
        });
}

} // namespace ibm_mc
} // namespace crow
