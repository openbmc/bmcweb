#pragma once

#include "webroutes.hpp"

#include <app_class_decl.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <http_request_class_decl.hpp>
#include <http_response_class_decl.hpp>
#include <routing_class_decl.hpp>
#include "forward_unauthorized.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace crow
{
namespace webassets
{

struct CmpStr
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

void requestRoutes(App& app);

} // namespace webassets
} // namespace crow
