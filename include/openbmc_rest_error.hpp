#pragma once

#include <string>

#include "crow/http_response.h"
#include "crow/http_request.h"

const std::string notFoundMsg = "404 Not Found";
const std::string badReqMsg = "400 Bad Request";
const std::string methodNotAllowedMsg = "405 Method Not Allowed";
const std::string forbiddenMsg = "403 Forbidden";
const std::string methodFailedMsg = "500 Method Call Failed";

const std::string notFoundDesc =
    "org.freedesktop.DBus.Error.FileNotFound: path or object not found";
const std::string propNotFoundDesc = "The specified property cannot be found";
const std::string noJsonDesc = "No JSON object could be decoded";
const std::string methodNotFoundDesc = "The specified method cannot be found";
const std::string methodNotAllowedDesc = "Method not allowed";
const std::string forbiddenPropDesc =
    "The specified property cannot be created";
const std::string forbiddenResDesc = "The specified resource cannot be created";

void setErrorResponse(crow::Response &res, boost::beast::http::status result,
                      const std::string &desc, const std::string &msg)
{
    res.result(result);
    res.jsonValue = {{"data", {{"description", desc}}},
                     {"message", msg},
                     {"status", "error"}};
}
