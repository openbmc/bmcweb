#pragma once

#include <experimental/filesystem>
#include <fstream>
#include <string>
#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/routing.h>
#include <crow/http_codes.h>
#include <boost/algorithm/string/replace.hpp>
#include <boost/container/flat_set.hpp>

namespace crow {
namespace webassets {

namespace filesystem = std::experimental::filesystem;

struct cmp_str {
  bool operator()(const char* a, const char* b) const {
    return std::strcmp(a, b) < 0;
  }
};

static boost::container::flat_set<std::string> routes;

template <typename... Middlewares>
void request_routes(Crow<Middlewares...>& app) {
  const static boost::container::flat_map<const char*, const char*, cmp_str>
      content_types{
          {{".css", "text/css;charset=UTF-8"},
           {".html", "text/html;charset=UTF-8"},
           {".js", "text/html;charset=UTF-8"},
           {".png", "image/png;charset=UTF-8"},
           {".woff", "application/x-font-woff"},
           {".woff2", "application/x-font-woff2"},
           {".gif", "image/gif"},
           {".ico", "image/x-icon"},
           {".ttf", "application/x-font-ttf"},
           {".svg", "image/svg+xml"},
           {".eot", "application/vnd.ms-fontobject"},
           {".xml", "application/xml"},
           // dev tools don't care about map type, setting to json causes
           // browser to show as text
           // https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
           {".map", "application/json"}}};
  auto rootpath = filesystem::path("/usr/share/www/");
  auto dir_iter = filesystem::recursive_directory_iterator(rootpath);
  for (auto& dir : dir_iter) {
    auto absolute_path = dir.path();
    auto relative_path = filesystem::path(
        absolute_path.string().substr(rootpath.string().size() - 1));
    // make sure we don't recurse into certain directories
    // note: maybe check for is_directory() here as well...

    if (filesystem::is_directory(dir)) {
      // don't recurse into hidden directories or symlinks
      if (boost::starts_with(dir.path().filename().string(), ".") ||
          filesystem::is_symlink(dir)) {
        dir_iter.disable_recursion_pending();
      }
    } else if (filesystem::is_regular_file(dir)) {
      std::string extension = relative_path.extension();
      auto webpath = relative_path;
      const char* content_encoding = nullptr;
      bool is_gzip = false;

      if (webpath.filename() == "$metadata") {
        // TODO, this endpoint should really be generated based on the redfish
        // data.  Once that is done, remove the type hardcode.
        extension = ".xml";
      }

      if (extension == ".gz") {
        webpath = webpath.replace_extension("");
        // Use the non-gzip version for determining content type
        extension = webpath.extension().string();
        content_encoding = "gzip";
        is_gzip = true;
      }

      if (webpath.filename() == "index.html") {
        webpath = webpath.parent_path();
        if (webpath.string() != "/") {
          webpath += "/";
        }
      }

      routes.insert(webpath.string());

      const char* content_type = nullptr;

      // if the filename has an extension, look up the type
      if (extension != "") {
        auto content_type_it = content_types.find(extension.c_str());

        if (content_type_it == content_types.end()) {
          CROW_LOG_ERROR << "Cannot determine content-type for " << webpath;
          continue;
        }
        content_type = content_type_it->second;
      }

      app.route_dynamic(webpath.string())(
          [absolute_path, content_type, content_encoding](
              const crow::request& req, crow::response& res) {
            if (content_type != nullptr) {
              res.add_header("Content-Type", content_type);
            }

            if (content_encoding != nullptr) {
              res.add_header("Content-Encoding", content_encoding);
            }

            // res.set_header("Cache-Control", "public, max-age=86400");
            std::ifstream inf(absolute_path);
            if (!inf) {
              CROW_LOG_DEBUG << "failed to read file";
	      res.code = static_cast<int>(HttpRespCode::NOT_FOUND);
              res.code = static_cast<int>(HttpRespCode::INTERNAL_ERROR);
              res.end();
              return;
            }

            res.body = {std::istreambuf_iterator<char>(inf),
                        std::istreambuf_iterator<char>()};
            res.end();
          });
    }
  }
}  // namespace webassets
}  // namespace webassets
}  // namespace crow
