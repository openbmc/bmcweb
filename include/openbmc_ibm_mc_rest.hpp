#pragma once
#include <app.h>
#include <mimetic/mimetic.h>
#include <tinyxml2.h>

#include <async_resp.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <filesystem>
#include <fstream>
#include <regex>
#include <sdbusplus/message/types.hpp>

using namespace mimetic;

namespace crow
{
namespace openbmc_ibm_mc
{

const std::string methodNotAllowedDesc = "Method not allowed";
const std::string methodNotAllowedMsg = "405 Method Not Allowed";

void setErrorResponse(crow::Response &res, boost::beast::http::status result,
                      const std::string &desc, const std::string &msg)
{
    res.result(result);
    res.jsonValue = {{"data", {{"description", desc}}},
                     {"message", msg},
                     {"status", "error"}};
}

void parseAndCreateFile(MimeEntity *pMe)
{
    BMCWEB_LOG_DEBUG << "parseAndCreateFile";
    Header &h = pMe->header();

    // create the file
    std::string fileId =
        h.contentDisposition().ContentDisposition::param("name");
    BMCWEB_LOG_DEBUG << "Save-area file name : " << fileId;

    std::ofstream file;
    std::filesystem::path loc("/var/lib/obmc/bmc-console-mgmt/save-area");
    file.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    loc /= fileId;
    try
    {
        BMCWEB_LOG_DEBUG << "Creating file " << loc;
        file.open(loc, std::ofstream::out);
        file << pMe->body();
        file.close();
        BMCWEB_LOG_DEBUG << "save-area file is created";
    }
    catch (std::ofstream::failure &e)
    {
        BMCWEB_LOG_DEBUG << "Error while opening the file for writing";
        return;
    }

    // Iterate over the data to create next files
    MimeEntityList &parts = pMe->body().parts(); // list of sub entities obj
    MimeEntityList::iterator mbit = parts.begin(), meit = parts.end();
    for (; mbit != meit; ++mbit)
    {
        parseAndCreateFile(*mbit);
    }
}
void handleFilePost(const crow::Request &req, crow::Response &res,
                    const std::string &objectPath,
                    const std::string &destProperty)
{
    BMCWEB_LOG_DEBUG << "handleIbmPost: Request to create the save-area file";

    // Check the content-type of the request
    std::string_view contentType = req.getHeaderValue("content-type");
    if (boost::starts_with(contentType, "multipart/form-data"))
    {
        BMCWEB_LOG_DEBUG << "This is multipart/form-data. Continue parsing";
    }
    else
    {
        BMCWEB_LOG_DEBUG
            << "Not a multipart/form-data. Wrong content-type sent";
        return;
    }

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
        res.end();
        BMCWEB_LOG_DEBUG << "handleIbmPost: Failed to prepare save-area dir";
        return;
    }

    // Using the req.body and the req header, prepare the mimetic parsable data
    std::string data = std::move(req.body);
    std::string contentline =
        "Content-Type: " + std::string(contentType) + "\n" + "\n";
    data.insert(0, contentline);

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
    parseAndCreateFile(&me);

    fin.close();
    return;
}

inline void handleFileUrl(const crow::Request &req, crow::Response &res,
                          std::string &objectPath)
{
    std::string destProperty = "";
    if (req.method() == "POST"_method)
    {
        handleFilePost(req, res, objectPath, destProperty);
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
    BMCWEB_ROUTE(app, "/ibm/v1/files/<path>")
        .requires({"ConfigureComponents", "ConfigureManager"})
        .methods("POST"_method, "DELETE"_method)([](const crow::Request &req,
                                                    crow::Response &res,
                                                    const std::string &path) {
            std::string objectPath = "/ibm/v1/files/" + path;
            handleFileUrl(req, res, objectPath);
        });
}
} // namespace openbmc_ibm_mc
} // namespace crow
