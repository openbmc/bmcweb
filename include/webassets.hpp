#pragma once

#include "filesystem.hpp"

#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/routing.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>
#include <fstream>
#include <string>

namespace crow
{
namespace webassets
{

namespace filesystem = std::filesystem;

struct CmpStr
{
    bool operator()(const char* a, const char* b) const
    {
        return std::strcmp(a, b) < 0;
    }
};

static boost::container::flat_set<std::string> routes;

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...>& app)
{
    const static boost::container::flat_map<const char*, const char*, CmpStr>
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
             {".json", "application/json"},
             // dev tools don't care about map type, setting to json causes
             // browser to show as text
             // https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
             {".map", "application/json"}}};
    filesystem::path rootpath{"/usr/share/www/"};
    filesystem::recursive_directory_iterator dirIter(rootpath);
    // In certain cases, we might have both a gzipped version of the file AND a
    // non-gzipped version.  To avoid duplicated routes, we need to make sure we
    // get the gzipped version first.  Because the gzipped path should be longer
    // than the non gzipped path, if we sort in descending order, we should be
    // guaranteed to get the gzip version first.
    std::vector<filesystem::directory_entry> paths(filesystem::begin(dirIter),
                                                   filesystem::end(dirIter));
    std::sort(paths.rbegin(), paths.rend());

    for (const filesystem::directory_entry& dir : paths)
    {
        filesystem::path absolutePath = dir.path();
        filesystem::path relativePath{
            absolutePath.string().substr(rootpath.string().size() - 1)};
        if (filesystem::is_directory(dir))
        {
            // don't recurse into hidden directories or symlinks
            if (boost::starts_with(dir.path().filename().string(), ".") ||
                filesystem::is_symlink(dir))
            {
                dirIter.disable_recursion_pending();
            }
        }
        else if (filesystem::is_regular_file(dir))
        {
            std::string extension = relativePath.extension();
            filesystem::path webpath = relativePath;
            const char* contentEncoding = nullptr;

            if (extension == ".gz")
            {
                webpath = webpath.replace_extension("");
                // Use the non-gzip version for determining content type
                extension = webpath.extension().string();
                contentEncoding = "gzip";
            }

            if (boost::starts_with(webpath.filename().string(), "index."))
            {
                webpath = webpath.parent_path();
                if (webpath.string().size() == 0 ||
                    webpath.string().back() != '/')
                {
                    // insert the non-directory version of this path
                    routes.insert(webpath);
                    webpath += "/";
                }
            }

            std::pair<boost::container::flat_set<std::string>::iterator, bool>
                inserted = routes.insert(webpath);

            if (!inserted.second)
            {
                // Got a duplicated path.  This is expected in certain
                // situations
                BMCWEB_LOG_DEBUG << "Got duplicated path " << webpath;
                continue;
            }
            const char* contentType = nullptr;

            auto contentTypeIt = contentTypes.find(extension.c_str());
            if (contentTypeIt == contentTypes.end())
            {
                BMCWEB_LOG_ERROR << "Cannot determine content-type for "
                                 << absolutePath << " with extension "
                                 << extension;
            }
            else
            {
                contentType = contentTypeIt->second;
            }

            app.routeDynamic(webpath)(
                [absolutePath, contentType, contentEncoding](
                    const crow::Request& req, crow::Response& res) {
                    if (contentType != nullptr)
                    {
                        res.addHeader("Content-Type", contentType);
                    }

                    if (contentEncoding != nullptr)
                    {
                        res.addHeader("Content-Encoding", contentEncoding);
                    }

                    // res.set_header("Cache-Control", "public, max-age=86400");
                    std::ifstream inf(absolutePath);
                    if (!inf)
                    {
                        BMCWEB_LOG_DEBUG << "failed to read file";
                        res.result(
                            boost::beast::http::status::internal_server_error);
                        res.end();
                        return;
                    }

                    res.body() = {std::istreambuf_iterator<char>(inf),
                                  std::istreambuf_iterator<char>()};
                    res.end();
                });
        }
    }
} // namespace webassets
} // namespace webassets
} // namespace crow
