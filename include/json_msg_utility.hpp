#pragma once

#include "crow/http_response.h"

namespace crow
{
namespace msg
{
const std::string notFoundMsg = "404 Not Found";
const std::string badReqMsg = "400 Bad Request";
const std::string methodNotAllowedMsg = "405 Method Not Allowed";
const std::string forbiddenMsg = "403 Forbidden";
const std::string methodFailedMsg = "500 Method Call Failed";
const std::string methodOutputFailedMsg = "500 Method Output Error";
const std::string unAuthMsg = "401 Unauthorized"; // Unauthenticated

const std::string notFoundDesc =
    "org.freedesktop.DBus.Error.FileNotFound: path or object not found";
const std::string propNotFoundDesc = "The specified property cannot be found";
const std::string noJsonDesc = "No JSON object could be decoded";
const std::string methodNotFoundDesc = "The specified method cannot be found";
const std::string methodNotAllowedDesc = "Method not allowed";
const std::string forbiddenPropDesc =
    "The specified property cannot be created";
const std::string forbiddenResDesc = "The specified resource cannot be created";
const std::string unAuthDesc = "The authentication could not be applied";
const std::string forbiddenURIDesc = "The session is not authorized to access URI: ";

} // namespace msg

inline void setErrorResponse(crow::Response &res,
                             boost::beast::http::status result,
                             const std::string &desc, const std::string &msg)
{
    res.result(result);
    res.jsonValue = {{"data", {{"description", desc}}},
                     {"message", msg},
                     {"status", "error"}};
}

inline void setPasswordChangeRequired(crow::Response &res,
                                      const std::string &username)
{
    res.jsonValue["extendedMessage"] =
        "The password for this account must be changed.  PATCH the 'Password' "
        "property for the account under URI: "
        "/redfish/v1/AccountService/Accounts/" +
        username;
}

} // namespace crow
