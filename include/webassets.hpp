#pragma once

#include "app.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "routing.hpp"
#include "webroutes.hpp"

#include <boost/algorithm/string/replace.hpp>

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

inline void requestRoutes(App& app)
{
    constexpr static std::array<std::pair<const char*, const char*>, 17>
        contentTypes{
            {{".css", "text/css;charset=UTF-8"},
             {".html", "text/html;charset=UTF-8"},
             {".js", "application/javascript;charset=UTF-8"},
             {".png", "image/png;charset=UTF-8"},
             {".woff", "application/x-font-woff"},
             {".woff2", "application/x-font-woff2"},
             {".gif", "image/gif"},
             {".ico", "image/x-icon"},
             {".ttf", "application/x-font-ttf"},
             {".svg", "image/svg+xml"},
             {".eot", "application/vnd.ms-fontobject"},
             {".xml", "application/xml"},
             {".json", "application/json"},
             {".jpg", "image/jpeg"},
             {".jpeg", "image/jpeg"},
             // dev tools don't care about map type, setting to json causes
             // browser to show as text
             // https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
             {".map", "application/json"}}};

    std::filesystem::path rootpath{"/usr/share/www/"};

    std::error_code ec;

    std::filesystem::recursive_directory_iterator dirIter(rootpath, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR << "Unable to find or open " << rootpath.string()
                         << " static file hosting disabled";
        return;
    }

    // In certain cases, we might have both a gzipped version of the file AND a
    // non-gzipped version.  To avoid duplicated routes, we need to make sure we
    // get the gzipped version first.  Because the gzipped path should be longer
    // than the non gzipped path, if we sort in descending order, we should be
    // guaranteed to get the gzip version first.
    std::vector<std::filesystem::directory_entry> paths(
        std::filesystem::begin(dirIter), std::filesystem::end(dirIter));
    std::sort(paths.rbegin(), paths.rend());

    for (const std::filesystem::directory_entry& dir : paths)
    {
        const std::filesystem::path& absolutePath = dir.path();
        std::filesystem::path relativePath{
            absolutePath.string().substr(rootpath.string().size() - 1)};
        if (std::filesystem::is_directory(dir))
        {
            // don't recurse into hidden directories or symlinks
            if (dir.path().filename().string().starts_with(".") ||
                std::filesystem::is_symlink(dir))
            {
                dirIter.disable_recursion_pending();
            }
        }
        else if (std::filesystem::is_regular_file(dir))
        {
            std::string extension = relativePath.extension();
            std::filesystem::path webpath = relativePath;
            const char* contentEncoding = nullptr;

            if (extension == ".gz")
            {
                webpath = webpath.replace_extension("");
                // Use the non-gzip version for determining content type
                extension = webpath.extension().string();
                contentEncoding = "gzip";
            }

            if (webpath.filename().string().starts_with("index."))
            {
                webpath = webpath.parent_path();
                if (webpath.string().empty() || webpath.string().back() != '/')
                {
                    // insert the non-directory version of this path
                    webroutes::routes.emplace_back(webpath);
                    webpath += "/";
                }
            }

            if (std::find(webroutes::routes.begin(), webroutes::routes.end(),
                          webpath) != webroutes::routes.end())
            {
                // Got a duplicated path.  This is expected in certain
                // situations
                BMCWEB_LOG_DEBUG << "Got duplicated path " << webpath.string();
                continue;
            }

            webroutes::routes.emplace_back(webpath);
            const char* contentType = nullptr;

            for (const std::pair<const char*, const char*>& ext : contentTypes)
            {
                if (ext.first == nullptr || ext.second == nullptr)
                {
                    continue;
                }
                if (extension == ext.first)
                {
                    contentType = ext.second;
                }
            }

            if (contentType == nullptr)
            {
                BMCWEB_LOG_ERROR << "Cannot determine content-type for "
                                 << absolutePath.string() << " with extension "
                                 << extension;
            }

            if (webpath == "/")
            {
                forward_unauthorized::hasWebuiRoute = true;
            }

            app.routeDynamic(webpath)(
                [absolutePath, contentType, contentEncoding](
                    const crow::Request&,
                    const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
                if (contentType != nullptr)
                {
                    asyncResp->res.addHeader(
                        boost::beast::http::field::content_type, contentType);
                }

                if (contentEncoding != nullptr)
                {
                    asyncResp->res.addHeader(
                        boost::beast::http::field::content_encoding,
                        contentEncoding);
                }

                // res.set_header("Cache-Control", "public, max-age=86400");
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
            });
        }
    }
}
} // namespace webassets
} // namespace crow
