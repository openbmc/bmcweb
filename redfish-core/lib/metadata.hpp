#pragma once
#include "app.hpp"
#include "async_resp.hpp"
#include "http_request.hpp"
#include "logging.hpp"
#include <tinyxml2.h>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/verb.hpp>
#include <filesystem>
#include <format>
#include <functional>
#include <memory>
#include <string>
#include <system_error>
#include <utility>

namespace redfish
{

std::string getMetadataPieceForFile(
    const std::filesystem::path& filename);
void requestRoutesMetadata(App& app);

} // namespace redfish