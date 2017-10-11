#include <crow/app.h>

#include <tinyxml2.h>
#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <dbus_singleton.hpp>

namespace crow {
namespace openbmc_mapper {

// TODO(ed) having these as scope globals, and as simple as they are limits the
// ability to queue multiple async operations at once.  Being able to register
// "done" callbacks to a queue here that also had a count attached would allow
// multiple requests to be running at once
std::atomic<std::size_t> outstanding_async_calls(0);
nlohmann::json object_paths;

void introspect_objects(crow::response &res, std::string process_name,
                        std::string path) {
  dbus::endpoint introspect_endpoint(
      process_name, path, "org.freedesktop.DBus.Introspectable", "Introspect");
  outstanding_async_calls++;
  crow::connections::system_bus->async_method_call(
      [&, process_name{std::move(process_name)}, object_path{std::move(path)} ](
          const boost::system::error_code ec,
          const std::string &introspect_xml) {
        outstanding_async_calls--;
        if (ec) {
          std::cerr << "Introspect call failed with error: " << ec.message()
                    << " on process: " << process_name
                    << " path: " << object_path << "\n";

        } else {
          object_paths.push_back({{"path", object_path}});

          tinyxml2::XMLDocument doc;

          doc.Parse(introspect_xml.c_str());
          tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
          if (pRoot == nullptr) {
            std::cerr << "XML document failed to parse " << process_name << " "
                      << path << "\n";

          } else {
            tinyxml2::XMLElement *node = pRoot->FirstChildElement("node");
            while (node != nullptr) {
              std::string child_path = node->Attribute("name");
              std::string newpath;
              if (object_path != "/") {
                newpath += object_path;
              }
              newpath += "/" + child_path;
              // intropect the subobjects as well
              introspect_objects(res, process_name, newpath);

              node = node->NextSiblingElement("node");
            }
          }
        }
        // if we're the last outstanding caller, finish the request
        if (outstanding_async_calls == 0) {
          nlohmann::json j{{"status", "ok"},
                           {"bus_name", process_name},
                           {"objects", object_paths}};

          res.write(j.dump());
          object_paths.clear();
          res.end();
        }
      },
      introspect_endpoint);
}

template <typename... Middlewares>
void request_routes(Crow<Middlewares...> &app) {
  CROW_ROUTE(app, "/bus/").methods("GET"_method)([](const crow::request &req) {
    return nlohmann::json{{"busses", {{{"name", "system"}}}}, {"status", "ok"}};

  });

  CROW_ROUTE(app, "/bus/system/")
      .methods("GET"_method)([](const crow::request &req, crow::response &res) {
        crow::connections::system_bus->async_method_call(
            [&](const boost::system::error_code ec,
                std::vector<std::string> &names) {
              std::sort(names.begin(), names.end());
              if (ec) {
                res.code = 500;
              } else {
                nlohmann::json j{{"status", "ok"}};
                auto &objects_sub = j["objects"];
                for (auto &name : names) {
                  objects_sub.push_back({{"name", name}});
                }

                res.write(j.dump());
              }

              res.end();

            },
            {"org.freedesktop.DBus", "/", "org.freedesktop.DBus", "ListNames"});

      });

  CROW_ROUTE(app, "/list/")
      .methods("GET"_method)([](const crow::request &req, crow::response &res) {
        crow::connections::system_bus->async_method_call(
            [&](const boost::system::error_code ec,
                const std::vector<std::string> &object_paths) {

              if (ec) {
                res.code = 500;
              } else {
                nlohmann::json j{{"status", "ok"},
                                 {"message", "200 OK"},
                                 {"data", object_paths}};
                res.body = j.dump();
              }
              res.end();
            },
            {"xyz.openbmc_project.ObjectMapper",
             "/xyz/openbmc_project/object_mapper",
             "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths"},
            "", static_cast<int32_t>(99), std::array<std::string, 0>());
      });

  CROW_ROUTE(app, "/xyz/<path>")
      .methods("GET"_method)([](const crow::request &req, crow::response &res,
                                const std::string &path) {
        if (outstanding_async_calls != 0) {
          res.code = 500;
          res.body = "request in progress";
          res.end();
          return;
        }
        using GetObjectType =
            std::vector<std::pair<std::string, std::vector<std::string>>>;

        std::string object_path = "/xyz/" + path;
        crow::connections::system_bus->async_method_call(
            // object_path intentially captured by value
            [&, object_path](const boost::system::error_code ec,
                             const GetObjectType &object_names) {

              if (ec) {
                res.code = 500;
                res.end();
                return;
              }
              if (object_names.size() != 1) {
                res.code = 404;
                res.end();
                return;
              }

              for (auto &interface : object_names[0].second) {
                outstanding_async_calls++;
                crow::connections::system_bus->async_method_call(
                    [&](const boost::system::error_code ec,
                        const std::vector<std::pair<
                            std::string, dbus::dbus_variant>> &properties) {
                      outstanding_async_calls--;
                      if (ec) {
                        std::cerr << "Bad dbus request error: " << ec;
                      } else {
                        for (auto &property : properties) {
                          boost::apply_visitor(
                              [&](auto val) {
                                object_paths[property.first] = val;
                              },
                              property.second);
                        }
                      }
                      if (outstanding_async_calls == 0) {
                        nlohmann::json j{{"status", "ok"},
                                         {"message", "200 OK"},
                                         {"data", object_paths}};
                        res.body = j.dump();
                        res.end();
                        object_paths.clear();
                      }
                    },
                    {object_names[0].first, object_path,
                     "org.freedesktop.DBus.Properties", "GetAll"},
                    interface);
              }
            },
            {"xyz.openbmc_project.ObjectMapper",
             "/xyz/openbmc_project/object_mapper",
             "xyz.openbmc_project.ObjectMapper", "GetObject"},
            object_path, std::array<std::string, 0>());
      });
  CROW_ROUTE(app, "/bus/system/<str>/")
      .methods("GET"_method)([](const crow::request &req, crow::response &res,
                                const std::string &connection) {
        if (outstanding_async_calls != 0) {
          res.code = 500;
          res.body = "request in progress";
          res.end();
          return;
        }
        introspect_objects(res, connection, "/");
      });

  CROW_ROUTE(app, "/bus/system/<str>/<path>")
      .methods("GET"_method)([](const crow::request &req, crow::response &res,
                                const std::string &process_name,
                                const std::string &requested_path) {

        std::vector<std::string> strs;
        boost::split(strs, requested_path, boost::is_any_of("/"));
        std::string object_path;
        std::string interface_name;
        std::string method_name;
        auto it = strs.begin();
        if (it == strs.end()) {
          object_path = "/";
        }
        while (it != strs.end()) {
          // Check if segment contains ".".  If it does, it must be an
          // interface
          if ((*it).find(".") != std::string::npos) {
            break;
            // THis check is neccesary as the trailing slash gets parsed as
            // part
            // of our <path> specifier above, which causes the normal trailing
            // backslash redirector to fail.
          } else if (!it->empty()) {
            object_path += "/" + *it;
          }
          it++;
        }
        if (it != strs.end()) {
          interface_name = *it;
          it++;

          // after interface, we might have a method name
          if (it != strs.end()) {
            method_name = *it;
            it++;
          }
        }
        if (it != strs.end()) {
          // if there is more levels past the method name, something went
          // wrong, throw an error
          res.code = 404;
          res.end();
          return;
        }
        dbus::endpoint introspect_endpoint(
            process_name, object_path, "org.freedesktop.DBus.Introspectable",
            "Introspect");
        if (interface_name.empty()) {
          crow::connections::system_bus->async_method_call(
              [
                    &, process_name{std::move(process_name)},
                    object_path{std::move(object_path)}
              ](const boost::system::error_code ec,
                const std::string &introspect_xml) {
                if (ec) {
                  std::cerr
                      << "Introspect call failed with error: " << ec.message()
                      << " on process: " << process_name
                      << " path: " << object_path << "\n";

                } else {
                  tinyxml2::XMLDocument doc;

                  doc.Parse(introspect_xml.c_str());
                  tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                  if (pRoot == nullptr) {
                    std::cerr << "XML document failed to parse " << process_name
                              << " " << object_path << "\n";
                    res.write(nlohmann::json{{"status", "XML parse error"}});
                    res.code = 500;
                  } else {
                    nlohmann::json interfaces_array = nlohmann::json::array();
                    tinyxml2::XMLElement *interface =
                        pRoot->FirstChildElement("interface");

                    while (interface != nullptr) {
                      std::string iface_name = interface->Attribute("name");
                      interfaces_array.push_back({{"name", iface_name}});

                      interface = interface->NextSiblingElement("interface");
                    }
                    nlohmann::json j{{"status", "ok"},
                                     {"bus_name", process_name},
                                     {"interfaces", interfaces_array},
                                     {"object_path", object_path}};
                    res.write(j.dump());
                  }
                }
                res.end();
              },
              introspect_endpoint);
        } else {
          crow::connections::system_bus->async_method_call(
              [
                    &, process_name{std::move(process_name)},
                    interface_name{std::move(interface_name)},
                    object_path{std::move(object_path)}
              ](const boost::system::error_code ec,
                const std::string &introspect_xml) {
                if (ec) {
                  std::cerr
                      << "Introspect call failed with error: " << ec.message()
                      << " on process: " << process_name
                      << " path: " << object_path << "\n";

                } else {
                  tinyxml2::XMLDocument doc;

                  doc.Parse(introspect_xml.c_str());
                  tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                  if (pRoot == nullptr) {
                    std::cerr << "XML document failed to parse " << process_name
                              << " " << object_path << "\n";
                    res.code = 500;

                  } else {
                    tinyxml2::XMLElement *node =
                        pRoot->FirstChildElement("node");

                    // if we know we're the only call, build the json directly
                    nlohmann::json methods_array = nlohmann::json::array();
                    nlohmann::json signals_array = nlohmann::json::array();
                    tinyxml2::XMLElement *interface =
                        pRoot->FirstChildElement("interface");

                    while (interface != nullptr) {
                      std::string iface_name = interface->Attribute("name");

                      if (iface_name == interface_name) {
                        tinyxml2::XMLElement *methods =
                            interface->FirstChildElement("method");
                        while (methods != nullptr) {
                          nlohmann::json args_array = nlohmann::json::array();
                          tinyxml2::XMLElement *arg =
                              methods->FirstChildElement("arg");
                          while (arg != nullptr) {
                            args_array.push_back(
                                {{"name", arg->Attribute("name")},
                                 {"type", arg->Attribute("type")},
                                 {"direction", arg->Attribute("direction")}});
                            arg = arg->NextSiblingElement("arg");
                          }
                          methods_array.push_back(
                              {{"name", methods->Attribute("name")},
                               {"uri",
                                "/bus/system/" + process_name + object_path +
                                    "/" + interface_name + "/" +
                                    methods->Attribute("name")},
                               {"args", args_array}});
                          methods = methods->NextSiblingElement("method");
                        }
                        tinyxml2::XMLElement *signals =
                            interface->FirstChildElement("signal");
                        while (signals != nullptr) {
                          nlohmann::json args_array = nlohmann::json::array();

                          tinyxml2::XMLElement *arg =
                              signals->FirstChildElement("arg");
                          while (arg != nullptr) {
                            std::string name = arg->Attribute("name");
                            std::string type = arg->Attribute("type");
                            args_array.push_back({
                                {"name", name}, {"type", type},
                            });
                            arg = arg->NextSiblingElement("arg");
                          }
                          signals_array.push_back(
                              {{"name", signals->Attribute("name")},
                               {"args", args_array}});
                          signals = signals->NextSiblingElement("signal");
                        }

                        nlohmann::json j{
                            {"status", "ok"},
                            {"bus_name", process_name},
                            {"interface", interface_name},
                            {"methods", methods_array},
                            {"object_path", object_path},
                            {"properties", nlohmann::json::object()},
                            {"signals", signals_array}};

                        res.write(j.dump());
                        break;
                      }

                      interface = interface->NextSiblingElement("interface");
                    }
                    if (interface == nullptr) {
                      // if we got to the end of the list and never found a
                      // match, throw 404
                      res.code = 404;
                    }
                  }
                }
                res.end();
              },
              introspect_endpoint);
        }

      });
}
}  // namespace openbmc_mapper
}  // namespace crow
