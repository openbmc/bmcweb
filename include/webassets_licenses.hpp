#pragma once

#include "webroutes.hpp"

#include <app.hpp>
#include <boost/container/flat_set.hpp>
#include <http_request.hpp>
#include <http_response.hpp>
#include <routing.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace crow
{
namespace webassets::licenses
{

inline void addPathToRoute(App& app, std::filesystem::path& webpath,
                           const std::filesystem::path& absolutePath)
{

    if (std::filesystem::is_directory(absolutePath))
    {
        // insert the non-directory version of this path
        webroutes::routes.insert(webpath);
        webpath += "/";
    }

    std::pair<boost::container::flat_set<std::string>::iterator, bool>
        inserted = webroutes::routes.insert(webpath);

    if (!inserted.second)
    {
        // Got a duplicated path.  This is expected in certain situations
        BMCWEB_LOG_DEBUG << "Got duplicated path " << webpath;
        return;
    }

    if (webpath == "/")
    {
        forward_unauthorized::hasWebuiRoute = true;
    }

    app.routeDynamic(
        webpath)([absolutePath](
                     const crow::Request&,
                     const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        asyncResp->res.addHeader("Content-Type", "text/plain");

        if (std::filesystem::is_directory(absolutePath))
        {
            std::error_code ec;
            std::filesystem::directory_iterator it(absolutePath, ec);
            if (ec)
            {
                BMCWEB_LOG_ERROR << "Unable to find or open " << absolutePath;
                return;
            }
            std::stringstream dirList;
            for (const std::filesystem::directory_entry& entry : it)
            {
                // don't display hidden directories or symlinks
                if (boost::starts_with(entry.path().filename().string(), ".") ||
                    std::filesystem::is_symlink(entry))
                {
                    continue;
                }
                dirList << entry.path().filename() << "\n";
            }

            asyncResp->res.body() = {std::istreambuf_iterator<char>(dirList),
                                     std::istreambuf_iterator<char>()};
        }
        else if (std::filesystem::is_regular_file(absolutePath))
        {
            std::ifstream inf(absolutePath);
            if (!inf)
            {
                BMCWEB_LOG_DEBUG << "failed to read file";
                asyncResp->res.result(
                    boost::beast::http::status::internal_server_error);
                return;
            }

            asyncResp->res.body() = {std::istreambuf_iterator<char>(inf),
                                     std::istreambuf_iterator<char>()};
        }
    });
}

inline void requestRoutes(App& app)
{

    std::filesystem::path rootpath{"/usr/share/common-licenses/"};
    std::filesystem::path rootWebpath{"/common-licenses"};
    addPathToRoute(app, rootWebpath, rootpath);

    std::error_code ec;

    std::filesystem::recursive_directory_iterator dirIter(rootpath, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Unable to find or open " << rootpath
                         << " static file hosting disabled";
        return;
    }

    for (const std::filesystem::directory_entry& dir : dirIter)
    {
        const std::filesystem::path& absolutePath = dir.path();
        std::filesystem::path relativePath{
            absolutePath.string().substr(rootpath.string().size() - 1)};
        if (boost::starts_with(dir.path().filename().string(), ".") ||
            std::filesystem::is_symlink(dir))
        {
            dirIter.disable_recursion_pending();
            continue;
        }
        std::filesystem::path webpath{rootWebpath /
                                      relativePath.relative_path()};

        addPathToRoute(app, webpath, absolutePath);
    }
}
} // namespace webassets::licenses
} // namespace crow
