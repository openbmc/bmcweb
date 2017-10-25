#pragma once

#include <experimental/filesystem>
#include <fstream>
#include <string>
#include <crow/app.h>
#include <crow/http_request.h>
#include <crow/http_response.h>
#include <crow/routing.h>
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
    auto absolute_path_str = dir.path().string();
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
      auto webpath = relative_path;
      bool is_gzip = false;
      if (relative_path.extension() == ".gz") {
        webpath = webpath.replace_extension("");
        is_gzip = true;
      }

      if (webpath.filename() == "index.html") {
        webpath = webpath.parent_path();
        if (webpath.string() != "/") {
          webpath += "/";
        }
      }

      routes.insert(webpath.string());

      std::string absolute_path_str = absolute_path.string();
      const char* content_type = nullptr;
      auto content_type_it = content_types.find(webpath.extension().c_str());
      if (content_type_it != content_types.end()) {
        content_type = content_type_it->second;
      } else {
        if (webpath.string() == "$metadata"){
          content_type_it = content_types.find(".xml");
          // should always be true
          if (content_type_it != content_types.end()) {
            content_type = content_type_it->second;
          }
        }
      }
      app.route_dynamic(std::string(webpath.string()))(
          [is_gzip, absolute_path_str, content_type](const crow::request& req,
                                                     crow::response& res) {
            static const char* content_type_string = "Content-Type";
            // std::string sha1("a576dc96a5c605b28afb032f3103630d61ac1068");
            // res.add_header("ETag", sha1);

            // if (req.get_header_value("If-None-Match") == sha1) {
            //  res.code = 304;
            //} else {
            //  res.code = 200;
            // TODO, if you have a browser from the dark ages that doesn't
            // support
            // gzip, unzip it before sending based on Accept-Encoding header
            //  res.add_header("Content-Encoding", gzip_string);
            if (content_type != nullptr) {
              res.add_header(content_type_string, content_type);
            }

            if (is_gzip) {
              res.add_header("Content-Encoding", "gzip");
            }

            // res.set_header("Cache-Control", "public, max-age=86400");
            std::ifstream inf(absolute_path_str);
            if (!inf) {
              CROW_LOG_DEBUG << "failed to read file";
              res.code = 400;
              res.end();
              return;
            }

            std::string body{std::istreambuf_iterator<char>(inf),
                             std::istreambuf_iterator<char>()};

            res.body = body;
            res.end();
          });
    }
  }
}
}  // namespace webassets
}  // namespace crow