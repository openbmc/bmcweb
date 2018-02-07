#include <crow/app.h>

#include <tinyxml2.h>
#include <dbus/connection.hpp>
#include <dbus/endpoint.hpp>
#include <dbus/filter.hpp>
#include <dbus/match.hpp>
#include <dbus/message.hpp>
#include <dbus_singleton.hpp>
#include <boost/container/flat_set.hpp>

namespace crow {
namespace openbmc_mapper {

void introspect_objects(crow::response &res, std::string process_name,
                        std::string path,
                        std::shared_ptr<nlohmann::json> transaction) {
  dbus::endpoint introspect_endpoint(
      process_name, path, "org.freedesktop.DBus.Introspectable", "Introspect");
  crow::connections::system_bus->async_method_call(
      [&, process_name{std::move(process_name)}, object_path{std::move(path)} ](
          const boost::system::error_code ec,
          const std::string &introspect_xml) {
        if (ec) {
          CROW_LOG_ERROR << "Introspect call failed with error: "
                         << ec.message() << " on process: " << process_name
                         << " path: " << object_path << "\n";

        } else {
          transaction->push_back({{"path", object_path}});

          tinyxml2::XMLDocument doc;

          doc.Parse(introspect_xml.c_str());
          tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
          if (pRoot == nullptr) {
            CROW_LOG_ERROR << "XML document failed to parse " << process_name
                           << " " << path << "\n";

          } else {
            tinyxml2::XMLElement *node = pRoot->FirstChildElement("node");
            while (node != nullptr) {
              std::string child_path = node->Attribute("name");
              std::string newpath;
              if (object_path != "/") {
                newpath += object_path;
              }
              newpath += "/" + child_path;
              // introspect the subobjects as well
              introspect_objects(res, process_name, newpath, transaction);

              node = node->NextSiblingElement("node");
            }
          }
        }
        // if we're the last outstanding caller, finish the request
        if (transaction.use_count() == 1) {
          res.json_value = {{"status", "ok"},
                            {"bus_name", process_name},
                            {"objects", *transaction}};
          res.end();
        }
      },
      introspect_endpoint);
}
using ManagedObjectType = std::vector<std::pair<
    dbus::object_path, boost::container::flat_map<
                           std::string, boost::container::flat_map<
                                            std::string, dbus::dbus_variant>>>>;

void get_manged_objects_for_enumerate(
    const std::string &object_name, const std::string &connection_name,
    crow::response &res, std::shared_ptr<nlohmann::json> transaction) {
  crow::connections::system_bus->async_method_call(
      [&res, transaction](const boost::system::error_code ec,
                          const ManagedObjectType &objects) {
        if (ec) {
          CROW_LOG_ERROR << ec;
        } else {
          nlohmann::json &data_json = *transaction;
          for (auto &object_path : objects) {
            nlohmann::json &object_json = data_json[object_path.first.value];
            for (const auto &interface : object_path.second) {
              for (const auto &property : interface.second) {
                boost::apply_visitor(
                    [&](auto &&val) { object_json[property.first] = val; },
                    property.second);
              }
            }
          }
        }

        if (transaction.use_count() == 1) {
          res.json_value = {{"message", "200 OK"},
                            {"status", "ok"},
                            {"data", std::move(*transaction)}};
          res.end();
        }
      },
      {connection_name, object_name, "org.freedesktop.DBus.ObjectManager",
       "GetManagedObjects"});
}  // namespace openbmc_mapper

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

void handle_enumerate(crow::response &res, const std::string &object_path) {
  crow::connections::system_bus->async_method_call(
      [&res, object_path{std::string(object_path)} ](
          const boost::system::error_code ec,
          const GetSubTreeType &object_names) {
        if (ec) {
          res.code = 500;
          res.end();
          return;
        }

        boost::container::flat_set<std::string> connections;

        for (const auto &object : object_names) {
          for (const auto &connection : object.second) {
            connections.insert(connection.first);
          }
        }

        if (connections.size() <= 0) {
          res.code = 404;
          res.end();
          return;
        }
        auto transaction =
            std::make_shared<nlohmann::json>(nlohmann::json::object());
        for (const std::string &connection : connections) {
          get_manged_objects_for_enumerate(object_path, connection, res,
                                           transaction);
        }

      },
      {"xyz.openbmc_project.ObjectMapper", "/xyz/openbmc_project/object_mapper",
       "xyz.openbmc_project.ObjectMapper", "GetSubTree"},
      object_path, (int32_t)0, std::array<std::string, 0>());
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

              if (ec) {
                res.code = 500;
              } else {
                std::sort(names.begin(), names.end());
                nlohmann::json j{{"status", "ok"}};
                auto &objects_sub = j["objects"];
                for (auto &name : names) {
                  objects_sub.push_back({{"name", name}});
                }
                res.json_value = std::move(j);
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
                res.json_value = {{"status", "ok"},
                                  {"message", "200 OK"},
                                  {"data", std::move(object_paths)}};
              }
              res.end();
            },
            {"xyz.openbmc_project.ObjectMapper",
             "/xyz/openbmc_project/object_mapper",
             "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths"},
            "", static_cast<int32_t>(99), std::array<std::string, 0>());
      });

  CROW_ROUTE(app, "/xyz/<path>")
      .methods("GET"_method,
               "PUT"_method)([](const crow::request &req, crow::response &res,
                                const std::string &path) {
        std::shared_ptr<nlohmann::json> transaction =
            std::make_shared<nlohmann::json>(nlohmann::json::object());
        using GetObjectType =
            std::vector<std::pair<std::string, std::vector<std::string>>>;
        std::string object_path;
        std::string dest_property;
        std::string property_set_value;
        size_t attr_position = path.find("/attr/");
        if (attr_position == path.npos) {
          object_path = "/xyz/" + path;
        } else {
          object_path = "/xyz/" + path.substr(0, attr_position);
          dest_property =
              path.substr((attr_position + strlen("/attr/")), path.length());
          auto request_dbus_data =
              nlohmann::json::parse(req.body, nullptr, false);
          if (request_dbus_data.is_discarded()) {
            res.code = 400;
            res.end();
            return;
          }

          auto property_value_it = request_dbus_data.find("data");
          if (property_value_it == request_dbus_data.end()) {
            res.code = 400;
            res.end();
            return;
          }

          property_set_value = property_value_it->get<const std::string>();
          if (property_set_value.empty()) {
            res.code = 400;
            res.end();
            return;
          }
        }

        if (boost::ends_with(object_path, "/enumerate")) {
          object_path.erase(object_path.end() - 10, object_path.end());
          handle_enumerate(res, object_path);
          return;
        }

        crow::connections::system_bus->async_method_call(
            [
                  &, object_path{std::move(object_path)},
                  dest_property{std::move(dest_property)},
                  property_set_value{std::move(property_set_value)}, transaction
            ](const boost::system::error_code ec,
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
              if (req.method == "GET"_method) {
                for (auto &interface : object_names[0].second) {
                  crow::connections::system_bus->async_method_call(
                      [&](const boost::system::error_code ec,
                          const std::vector<std::pair<
                              std::string, dbus::dbus_variant>> &properties) {
                        if (ec) {
                          CROW_LOG_ERROR << "Bad dbus request error: " << ec;
                        } else {
                          for (auto &property : properties) {
                            boost::apply_visitor(
                                [&](auto val) {
                                  (*transaction)[property.first] = val;
                                },
                                property.second);
                          }
                        }
                        if (transaction.use_count() == 1) {
                          res.json_value = {{"status", "ok"},
                                            {"message", "200 OK"},
                                            {"data", *transaction}};

                          res.end();
                        }
                      },
                      {object_names[0].first, object_path,
                       "org.freedesktop.DBus.Properties", "GetAll"},
                      interface);
                }
              } else if (req.method == "PUT"_method) {
                for (auto &interface : object_names[0].second) {
                  crow::connections::system_bus->async_method_call(
                      [
                            &, interface{std::move(interface)},
                            object_names{std::move(object_names)},
                            object_path{std::move(object_path)},
                            dest_property{std::move(dest_property)},
                            property_set_value{std::move(property_set_value)},
                            transaction
                      ](const boost::system::error_code ec,
                        const boost::container::flat_map<
                            std::string, dbus::dbus_variant> &properties) {
                        if (ec) {
                          CROW_LOG_ERROR << "Bad dbus request error: " << ec;
                        } else {
                          auto it = properties.find(dest_property);
                          if (it != properties.end()) {
                            // find the matched property in the interface
                            dbus::dbus_variant property_value(
                                property_set_value);  // create the dbus
                                                      // variant for dbus call
                            crow::connections::system_bus->async_method_call(
                                [&](const boost::system::error_code ec) {
                                  // use the method "Set" to set the property
                                  // value
                                  if (ec) {
                                    CROW_LOG_ERROR << "Bad dbus request error: "
                                                   << ec;
                                  }
                                  // find the matched property and send the
                                  // response
                                  *transaction = {{"status", "ok"},
                                                  {"message", "200 OK"},
                                                  {"data", nullptr}};

                                },
                                {object_names[0].first, object_path,
                                 "org.freedesktop.DBus.Properties", "Set"},
                                interface, dest_property, property_value);
                          }
                        }
                        // if we are the last caller, finish the transaction
                        if (transaction.use_count() == 1) {
                          // if nobody filled in the property, all calls either
                          // errored, or failed
                          if (transaction == nullptr) {
                            res.code = 403;
                            res.json_value = {{"status", "error"},
                                              {"message", "403 Forbidden"},
                                              {"data",
                                               {{"message",
                                                 "The specified property "
                                                 "cannot be created: " +
                                                     dest_property}}}};

                          } else {
                            res.json_value = *transaction;
                          }

                          res.end();
                          return;
                        }
                      },
                      {object_names[0].first, object_path,
                       "org.freedesktop.DBus.Properties", "GetAll"},
                      interface);
                }
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
        std::shared_ptr<nlohmann::json> transaction;
        introspect_objects(res, connection, "/", transaction);
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
            // part of our <path> specifier above, which causes the normal
            // trailing backslash redirector to fail.
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
                  CROW_LOG_ERROR
                      << "Introspect call failed with error: " << ec.message()
                      << " on process: " << process_name
                      << " path: " << object_path << "\n";

                } else {
                  tinyxml2::XMLDocument doc;

                  doc.Parse(introspect_xml.c_str());
                  tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                  if (pRoot == nullptr) {
                    CROW_LOG_ERROR << "XML document failed to parse "
                                   << process_name << " " << object_path
                                   << "\n";
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
                    res.json_value = {{"status", "ok"},
                                      {"bus_name", process_name},
                                      {"interfaces", interfaces_array},
                                      {"object_path", object_path}};
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
                  CROW_LOG_ERROR
                      << "Introspect call failed with error: " << ec.message()
                      << " on process: " << process_name
                      << " path: " << object_path << "\n";

                } else {
                  tinyxml2::XMLDocument doc;

                  doc.Parse(introspect_xml.c_str());
                  tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                  if (pRoot == nullptr) {
                    CROW_LOG_ERROR << "XML document failed to parse "
                                   << process_name << " " << object_path
                                   << "\n";
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
                               {"uri", "/bus/system/" + process_name +
                                           object_path + "/" + interface_name +
                                           "/" + methods->Attribute("name")},
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
                                {"name", name},
                                {"type", type},
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
