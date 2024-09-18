#pragma once

#include "app.hpp"
#include "http_request.hpp"
#include "http_response.hpp"
#include "routing.hpp"
#include "webroutes.hpp"

#include <boost/container/flat_set.hpp>

#include <algorithm>
#include <array>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace crow
{
namespace webassets
{

inline std::string getStaticEtag(const std::filesystem::path& webpath)
{
    // webpack outputs production chunks in the form:
    // <filename>.<hash>.<extension>
    // For example app.63e2c453.css
    // Try to detect this, so we can use the hash as the ETAG
    std::vector<std::string> split;
    bmcweb::split(split, webpath.filename().string(), '.');
    BMCWEB_LOG_DEBUG("Checking {} split.size() {}", webpath.filename().string(),
                     split.size());
    if (split.size() < 3)
    {
        return "";
    }

    // get the second to last element
    std::string hash = split.rbegin()[1];

    // Webpack hashes are 8 characters long
    if (hash.size() != 8)
    {
        return "";
    }
    // Webpack hashes only include hex printable characters
    if (hash.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
    {
        return "";
    }
    return std::format("\"{}\"", hash);
}

static constexpr std::string_view rootpath("/usr/share/www/");

struct StaticFile
{
    std::filesystem::path absolutePath;
    std::string_view contentType;
    std::string_view contentEncoding;
    std::string etag;
    bool renamed = false;
};

inline void
    handleStaticAsset(const crow::Request& req,
                      const std::shared_ptr<bmcweb::AsyncResp>& asyncResp,
                      const StaticFile& file)
{
    if (!file.contentType.empty())
    {
        asyncResp->res.addHeader(boost::beast::http::field::content_type,
                                 file.contentType);
    }

    if (!file.contentEncoding.empty())
    {
        asyncResp->res.addHeader(boost::beast::http::field::content_encoding,
                                 file.contentEncoding);
    }

    if (!file.etag.empty())
    {
        asyncResp->res.addHeader(boost::beast::http::field::etag, file.etag);
        // Don't cache paths that don't have the etag in them, like
        // index, which gets transformed to /
        if (!file.renamed)
        {
            // Anything with a hash can be cached forever and is
            // immutable
            asyncResp->res.addHeader(boost::beast::http::field::cache_control,
                                     "max-age=31556926, immutable");
        }

        std::string_view cachedEtag =
            req.getHeaderValue(boost::beast::http::field::if_none_match);
        if (cachedEtag == file.etag)
        {
            asyncResp->res.result(boost::beast::http::status::not_modified);
            return;
        }
    }

    if (asyncResp->res.openFile(file.absolutePath) != crow::OpenCode::Success)
    {
        BMCWEB_LOG_DEBUG("failed to read file");
        asyncResp->res.result(
            boost::beast::http::status::internal_server_error);
        return;
    }
}

inline std::string_view getFiletypeForExtension(std::string_view extension)
{
    constexpr static std::array<std::pair<std::string_view, std::string_view>,
                                17>
        contentTypes{
            {{".css", "text/css;charset=UTF-8"},
             {".eot", "application/vnd.ms-fontobject"},
             {".gif", "image/gif"},
             {".html", "text/html;charset=UTF-8"},
             {".ico", "image/x-icon"},
             {".jpeg", "image/jpeg"},
             {".jpg", "image/jpeg"},
             {".js", "application/javascript;charset=UTF-8"},
             {".json", "application/json"},
             // dev tools don't care about map type, setting to json causes
             // browser to show as text
             // https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
             {".map", "application/json"},
             {".png", "image/png;charset=UTF-8"},
             {".svg", "image/svg+xml"},
             {".ttf", "application/x-font-ttf"},
             {".woff", "application/x-font-woff"},
             {".woff2", "application/x-font-woff2"},
             {".xml", "application/xml"}}};

    const auto* contentType = std::ranges::find_if(
        contentTypes,
        [&extension](const auto& val) { return val.first == extension; });

    if (contentType == contentTypes.end())
    {
        BMCWEB_LOG_ERROR(
            "Cannot determine content-type for file with extension {}",
            extension);
        return "";
    }
    return contentType->second;
}

inline void addFile(App& app, const std::filesystem::directory_entry& dir)
{
    StaticFile file;
    file.absolutePath = dir.path();
    std::filesystem::path relativePath(
        file.absolutePath.string().substr(rootpath.size() - 1));

    std::string extension = relativePath.extension();
    std::filesystem::path webpath = relativePath;

    if (extension == ".gz")
    {
        webpath = webpath.replace_extension("");
        // Use the non-gzip version for determining content type
        extension = webpath.extension().string();
        file.contentEncoding = "gzip";
    }
    else if (extension == ".zstd")
    {
        webpath = webpath.replace_extension("");
        // Use the non-zstd version for determining content type
        extension = webpath.extension().string();
        file.contentEncoding = "zstd";
    }

    file.etag = getStaticEtag(webpath);

    if (webpath.filename().string().starts_with("index."))
    {
        webpath = webpath.parent_path();
        if (webpath.string().empty() || webpath.string().back() != '/')
        {
            // insert the non-directory version of this path
            webroutes::routes.insert(webpath);
            webpath += "/";
            file.renamed = true;
        }
    }

    std::pair<boost::container::flat_set<std::string>::iterator, bool>
        inserted = webroutes::routes.insert(webpath);

    if (!inserted.second)
    {
        // Got a duplicated path.  This is expected in certain
        // situations
        BMCWEB_LOG_DEBUG("Got duplicated path {}", webpath.string());
        return;
    }
    file.contentType = getFiletypeForExtension(extension);

    if (webpath == "/")
    {
        forward_unauthorized::hasWebuiRoute = true;
    }

    app.routeDynamic(webpath)(
        [file = std::move(file)](
            const crow::Request& req,
            const std::shared_ptr<bmcweb::AsyncResp>& asyncResp) {
        handleStaticAsset(req, asyncResp, file);
    });
}

inline void requestRoutes(App& app)
{
    std::error_code ec;
    std::filesystem::recursive_directory_iterator dirIter({rootpath}, ec);
    if (ec)
    {
        BMCWEB_LOG_ERROR(
            "Unable to find or open {} static file hosting disabled", rootpath);
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
            addFile(app, dir);
        }
    }
}
} // namespace webassets
} // namespace crow
