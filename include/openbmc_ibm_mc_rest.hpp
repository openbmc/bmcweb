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

namespace crow
{
namespace openbmc_ibm_mc
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
        res.result(boost::beast::http::status::internal_server_error);
        res.jsonValue["message"] = {internalServerError};
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

        res.result(boost::beast::http::status::not_acceptable);
        res.jsonValue["message"] = {contentNotAcceptableMsg};
        return;
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Not a multipart/form-data. Continue..";
    }
    std::size_t pos = objectPath.find("configFiles/");
    if (pos != std::string::npos)
    {
        BMCWEB_LOG_DEBUG
            << "handleIbmPut: Request to create/update the save-area file";
        if (!createSaveAreaPath(res))
        {
            res.result(boost::beast::http::status::not_found);
            res.jsonValue["message"] = {resourceNotFoundMsg};
            return;
        }
        // Extract the file name from the objectPath
        std::string fileId = objectPath.substr(pos + strlen("configFiles/"));
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
            res.result(boost::beast::http::status::internal_server_error);
            res.jsonValue["message"] = {"Error while creating the file"};
            return;
        }
    }
    else
    {
        BMCWEB_LOG_DEBUG << "Bad URI";
        res.result(boost::beast::http::status::not_found);
        res.jsonValue["message"] = {resourceNotFoundMsg};
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
    res.jsonValue["message"] = {methodNotAllowedMsg};
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
