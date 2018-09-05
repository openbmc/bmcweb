#include <crow/app.h>
#include <tinyxml2.h>

#include <boost/algorithm/string.hpp>
#include <boost/container/flat_set.hpp>
#include <dbus_singleton.hpp>
#include <experimental/filesystem>
#include <fstream>

namespace crow
{
namespace openbmc_mapper
{

void introspectObjects(crow::Response &res, std::string process_name,
                       std::string path,
                       std::shared_ptr<nlohmann::json> transaction)
{
    crow::connections::systemBus->async_method_call(
        [&res, transaction, processName{std::move(process_name)},
         objectPath{std::move(path)}](const boost::system::error_code ec,
                                      const std::string &introspect_xml) {
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Introspect call failed with error: " << ec.message()
                    << " on process: " << processName << " path: " << objectPath
                    << "\n";
            }
            else
            {
                transaction->push_back({{"path", objectPath}});

                tinyxml2::XMLDocument doc;

                doc.Parse(introspect_xml.c_str());
                tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                if (pRoot == nullptr)
                {
                    BMCWEB_LOG_ERROR << "XML document failed to parse "
                                     << processName << " " << objectPath
                                     << "\n";
                }
                else
                {
                    tinyxml2::XMLElement *node =
                        pRoot->FirstChildElement("node");
                    while (node != nullptr)
                    {
                        std::string childPath = node->Attribute("name");
                        std::string newpath;
                        if (objectPath != "/")
                        {
                            newpath += objectPath;
                        }
                        newpath += "/" + childPath;
                        // introspect the subobjects as well
                        introspectObjects(res, processName, newpath,
                                          transaction);

                        node = node->NextSiblingElement("node");
                    }
                }
            }
            // if we're the last outstanding caller, finish the request
            if (transaction.use_count() == 1)
            {
                res.jsonValue = {{"status", "ok"},
                                 {"bus_name", processName},
                                 {"objects", std::move(*transaction)}};
                res.end();
            }
        },
        process_name, path, "org.freedesktop.DBus.Introspectable",
        "Introspect");
}

// A smattering of common types to unpack.  TODO(ed) this should really iterate
// the sdbusplus object directly and build the json response
using DbusRestVariantType = sdbusplus::message::variant<
    std::vector<std::tuple<std::string, std::string, std::string>>, std::string,
    int64_t, uint64_t, double, int32_t, uint32_t, int16_t, uint16_t, uint8_t,
    bool>;

using ManagedObjectType = std::vector<std::pair<
    sdbusplus::message::object_path,
    boost::container::flat_map<
        std::string,
        boost::container::flat_map<std::string, DbusRestVariantType>>>>;

void getManagedObjectsForEnumerate(const std::string &object_name,
                                   const std::string &connection_name,
                                   crow::Response &res,
                                   std::shared_ptr<nlohmann::json> transaction)
{
    crow::connections::systemBus->async_method_call(
        [&res, transaction](const boost::system::error_code ec,
                            const ManagedObjectType &objects) {
            if (ec)
            {
                BMCWEB_LOG_ERROR << ec;
            }
            else
            {
                nlohmann::json &dataJson = *transaction;

                for (auto &objectPath : objects)
                {
                    BMCWEB_LOG_DEBUG
                        << "Reading object "
                        << static_cast<const std::string &>(objectPath.first);
                    nlohmann::json &objectJson =
                        dataJson[static_cast<const std::string &>(
                            objectPath.first)];
                    if (objectJson.is_null())
                    {
                        objectJson = nlohmann::json::object();
                    }
                    for (const auto &interface : objectPath.second)
                    {
                        for (const auto &property : interface.second)
                        {
                            nlohmann::json &propertyJson =
                                objectJson[property.first];
                            mapbox::util::apply_visitor(
                                [&propertyJson](auto &&val) {
                                    propertyJson = val;
                                },
                                property.second);
                        }
                    }
                }
            }

            if (transaction.use_count() == 1)
            {
                res.jsonValue = {{"message", "200 OK"},
                                 {"status", "ok"},
                                 {"data", std::move(*transaction)}};
                res.end();
            }
        },
        connection_name, object_name, "org.freedesktop.DBus.ObjectManager",
        "GetManagedObjects");
}

using GetSubTreeType = std::vector<
    std::pair<std::string,
              std::vector<std::pair<std::string, std::vector<std::string>>>>>;

// Structure for storing data on an in progress action
struct InProgressActionData
{
    InProgressActionData(crow::Response &res) : res(res){};
    ~InProgressActionData()
    {
        if (res.result() == boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            res.jsonValue = nlohmann::json::object();
        }
        res.end();
    }

    void setErrorStatus()
    {
        res.result(boost::beast::http::status::internal_server_error);
    }
    crow::Response &res;
    std::string path;
    std::string methodName;
    nlohmann::json arguments;
};

std::vector<std::string> dbusArgSplit(const std::string &string)
{
    std::vector<std::string> ret;
    if (string.empty())
    {
        return ret;
    }
    ret.push_back("");
    int containerDepth = 0;

    for (std::string::const_iterator character = string.begin();
         character != string.end(); character++)
    {
        ret.back() += *character;
        switch (*character)
        {
            case ('a'):
                break;
            case ('('):
            case ('{'):
                containerDepth++;
                break;
            case ('}'):
            case (')'):
                containerDepth--;
                if (containerDepth == 0)
                {
                    if (character + 1 != string.end())
                    {
                        ret.push_back("");
                    }
                }
                break;
            default:
                if (containerDepth == 0)
                {
                    if (character + 1 != string.end())
                    {
                        ret.push_back("");
                    }
                }
                break;
        }
    }
}

int convertJsonToDbus(sd_bus_message *m, const std::string &arg_type,
                      const nlohmann::json &input_json)
{
    int r = 0;
    BMCWEB_LOG_DEBUG << "Converting " << input_json.dump()
                     << " to type: " << arg_type;
    const std::vector<std::string> argTypes = dbusArgSplit(arg_type);

    // Assume a single object for now.
    const nlohmann::json *j = &input_json;
    nlohmann::json::const_iterator jIt = input_json.begin();

    for (const std::string &arg_code : argTypes)
    {
        // If we are decoding multiple objects, grab the pointer to the
        // iterator, and increment it for the next loop
        if (argTypes.size() > 1)
        {
            if (jIt == input_json.end())
            {
                return -2;
            }
            j = &*jIt;
            jIt++;
        }
        const int64_t *int_value = j->get_ptr<const int64_t *>();
        const uint64_t *uint_value = j->get_ptr<const uint64_t *>();
        const std::string *string_value = j->get_ptr<const std::string *>();
        const double *double_value = j->get_ptr<const double *>();
        const bool *b = j->get_ptr<const bool *>();
        int64_t v = 0;
        double d = 0.0;

        // Do some basic type conversions that make sense.  uint can be
        // converted to int.  int and uint can be converted to double
        if (uint_value != nullptr && int_value == nullptr)
        {
            v = static_cast<int64_t>(*uint_value);
            int_value = &v;
        }
        if (uint_value != nullptr && double_value == nullptr)
        {
            d = static_cast<double>(*uint_value);
            double_value = &d;
        }
        if (int_value != nullptr && double_value == nullptr)
        {
            d = static_cast<double>(*int_value);
            double_value = &d;
        }

        if (arg_code == "s")
        {
            if (string_value == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, arg_code[0],
                                            (void *)string_value->c_str());
            if (r < 0)
            {
                return r;
            }
        }
        else if (arg_code == "i")
        {
            if (int_value == nullptr)
            {
                return -1;
            }
            int32_t i = static_cast<int32_t>(*int_value);
            r = sd_bus_message_append_basic(m, arg_code[0], &i);
            if (r < 0)
            {
                return r;
            }
        }
        else if (arg_code == "b")
        {
            // lots of ways bool could be represented here.  Try them all
            int bool_int = false;
            if (int_value != nullptr)
            {
                bool_int = *int_value > 0 ? 1 : 0;
            }
            else if (b != nullptr)
            {
                bool_int = b ? 1 : 0;
            }
            else if (string_value != nullptr)
            {
                bool_int = boost::istarts_with(*string_value, "t") ? 1 : 0;
            }
            else
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, arg_code[0], &bool_int);
            if (r < 0)
            {
                return r;
            }
        }
        else if (arg_code == "n")
        {
            if (int_value == nullptr)
            {
                return -1;
            }
            int16_t n = static_cast<int16_t>(*int_value);
            r = sd_bus_message_append_basic(m, arg_code[0], &n);
            if (r < 0)
            {
                return r;
            }
        }
        else if (arg_code == "x")
        {
            if (int_value == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, arg_code[0], int_value);
            if (r < 0)
            {
                return r;
            }
        }
        else if (arg_code == "y")
        {
            if (uint_value == nullptr)
            {
                return -1;
            }
            uint8_t y = static_cast<uint8_t>(*uint_value);
            r = sd_bus_message_append_basic(m, arg_code[0], &y);
        }
        else if (arg_code == "q")
        {
            if (uint_value == nullptr)
            {
                return -1;
            }
            uint16_t q = static_cast<uint16_t>(*uint_value);
            r = sd_bus_message_append_basic(m, arg_code[0], &q);
        }
        else if (arg_code == "u")
        {
            if (uint_value == nullptr)
            {
                return -1;
            }
            uint32_t u = static_cast<uint32_t>(*uint_value);
            r = sd_bus_message_append_basic(m, arg_code[0], &u);
        }
        else if (arg_code == "t")
        {
            if (uint_value == nullptr)
            {
                return -1;
            }
            r = sd_bus_message_append_basic(m, arg_code[0], uint_value);
        }
        else if (arg_code == "d")
        {
            sd_bus_message_append_basic(m, arg_code[0], double_value);
        }
        else if (boost::starts_with(arg_code, "a"))
        {
            std::string contained_type = arg_code.substr(1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_ARRAY,
                                              contained_type.c_str());
            if (r < 0)
            {
                return r;
            }

            for (nlohmann::json::const_iterator it = j->begin(); it != j->end();
                 ++it)
            {
                r = convertJsonToDbus(m, contained_type, *it);
                if (r < 0)
                {
                    return r;
                }

                it++;
            }
            sd_bus_message_close_container(m);
        }
        else if (boost::starts_with(arg_code, "v"))
        {
            std::string contained_type = arg_code.substr(1);
            BMCWEB_LOG_DEBUG
                << "variant type: " << arg_code
                << " appending variant of type: " << contained_type;
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_VARIANT,
                                              contained_type.c_str());
            if (r < 0)
            {
                return r;
            }

            r = convertJsonToDbus(m, contained_type, input_json);
            if (r < 0)
            {
                return r;
            }

            r = sd_bus_message_close_container(m);
            if (r < 0)
            {
                return r;
            }
        }
        else if (boost::starts_with(arg_code, "(") &&
                 boost::ends_with(arg_code, ")"))
        {
            std::string contained_type =
                arg_code.substr(1, arg_code.size() - 1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_STRUCT,
                                              contained_type.c_str());
            nlohmann::json::const_iterator it = j->begin();
            for (const std::string &arg_code : dbusArgSplit(arg_type))
            {
                if (it == j->end())
                {
                    return -1;
                }
                r = convertJsonToDbus(m, arg_code, *it);
                if (r < 0)
                {
                    return r;
                }
                it++;
            }
            r = sd_bus_message_close_container(m);
        }
        else if (boost::starts_with(arg_code, "{") &&
                 boost::ends_with(arg_code, "}"))
        {
            std::string contained_type =
                arg_code.substr(1, arg_code.size() - 1);
            r = sd_bus_message_open_container(m, SD_BUS_TYPE_DICT_ENTRY,
                                              contained_type.c_str());
            std::vector<std::string> codes = dbusArgSplit(contained_type);
            if (codes.size() != 2)
            {
                return -1;
            }
            const std::string &key_type = codes[0];
            const std::string &value_type = codes[1];
            for (auto it : j->items())
            {
                r = convertJsonToDbus(m, key_type, it.key());
                if (r < 0)
                {
                    return r;
                }

                r = convertJsonToDbus(m, value_type, it.value());
                if (r < 0)
                {
                    return r;
                }
            }
            r = sd_bus_message_close_container(m);
        }
        else
        {
            return -2;
        }
        if (r < 0)
        {
            return r;
        }

        if (argTypes.size() > 1)
        {
            jIt++;
        }
    }
}

void findActionOnInterface(std::shared_ptr<InProgressActionData> transaction,
                           const std::string &connectionName)
{
    BMCWEB_LOG_DEBUG << "findActionOnInterface for connection "
                     << connectionName;
    crow::connections::systemBus->async_method_call(
        [transaction, connectionName{std::string(connectionName)}](
            const boost::system::error_code ec,
            const std::string &introspect_xml) {
            BMCWEB_LOG_DEBUG << "got xml:\n " << introspect_xml;
            if (ec)
            {
                BMCWEB_LOG_ERROR
                    << "Introspect call failed with error: " << ec.message()
                    << " on process: " << connectionName << "\n";
            }
            else
            {
                tinyxml2::XMLDocument doc;

                doc.Parse(introspect_xml.c_str());
                tinyxml2::XMLNode *pRoot = doc.FirstChildElement("node");
                if (pRoot == nullptr)
                {
                    BMCWEB_LOG_ERROR << "XML document failed to parse "
                                     << connectionName << "\n";
                }
                else
                {
                    tinyxml2::XMLElement *interface_node =
                        pRoot->FirstChildElement("interface");
                    while (interface_node != nullptr)
                    {
                        std::string this_interface_name =
                            interface_node->Attribute("name");
                        tinyxml2::XMLElement *method_node =
                            interface_node->FirstChildElement("method");
                        while (method_node != nullptr)
                        {
                            std::string this_methodName =
                                method_node->Attribute("name");
                            BMCWEB_LOG_DEBUG << "Found method: "
                                             << this_methodName;
                            if (this_methodName == transaction->methodName)
                            {
                                sdbusplus::message::message m =
                                    crow::connections::systemBus
                                        ->new_method_call(
                                            connectionName.c_str(),
                                            transaction->path.c_str(),
                                            this_interface_name.c_str(),
                                            transaction->methodName.c_str());

                                tinyxml2::XMLElement *argument_node =
                                    method_node->FirstChildElement("arg");

                                nlohmann::json::const_iterator arg_it =
                                    transaction->arguments.begin();

                                while (argument_node != nullptr)
                                {
                                    std::string arg_direction =
                                        argument_node->Attribute("direction");
                                    if (arg_direction == "in")
                                    {
                                        std::string arg_type =
                                            argument_node->Attribute("type");
                                        if (arg_it ==
                                            transaction->arguments.end())
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }
                                        if (convertJsonToDbus(m.get(), arg_type,
                                                              *arg_it) < 0)
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }

                                        arg_it++;
                                    }
                                    argument_node =
                                        method_node->NextSiblingElement("arg");
                                }
                                crow::connections::systemBus->async_send(
                                    m, [transaction](
                                           boost::system::error_code ec,
                                           sdbusplus::message::message &m) {
                                        if (ec)
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }
                                        transaction->res.jsonValue = {
                                            {"status", "ok"},
                                            {"message", "200 OK"},
                                            {"data", nullptr}};
                                    });
                                break;
                            }
                            method_node =
                                method_node->NextSiblingElement("method");
                        }
                        interface_node =
                            interface_node->NextSiblingElement("interface");
                    }
                }
            }
        },
        connectionName, transaction->path,
        "org.freedesktop.DBus.Introspectable", "Introspect");
}

void handle_action(const crow::Request &req, crow::Response &res,
                   const std::string &objectPath, const std::string &methodName)
{
    nlohmann::json requestDbusData =
        nlohmann::json::parse(req.body, nullptr, false);

    if (requestDbusData.is_discarded())
    {
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    if (!requestDbusData.is_array())
    {
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    auto transaction = std::make_shared<InProgressActionData>(res);

    transaction->path = objectPath;
    transaction->methodName = methodName;
    transaction->arguments = std::move(requestDbusData);
    crow::connections::systemBus->async_method_call(
        [transaction](
            const boost::system::error_code ec,
            const std::vector<std::pair<std::string, std::vector<std::string>>>
                &interface_names) {
            if (ec || interface_names.size() <= 0)
            {
                transaction->setErrorStatus();
                return;
            }

            BMCWEB_LOG_DEBUG << "GetObject returned objects "
                             << interface_names.size();

            for (const std::pair<std::string, std::vector<std::string>>
                     &object : interface_names)
            {
                findActionOnInterface(transaction, object.first);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", objectPath,
        std::array<std::string, 0>());
}

void handle_list(crow::Response &res, const std::string &objectPath)
{
    crow::connections::systemBus->async_method_call(
        [&res](const boost::system::error_code ec,
               std::vector<std::string> &objectPaths) {
            if (ec)
            {
                res.result(boost::beast::http::status::internal_server_error);
            }
            else
            {
                res.jsonValue = {{"status", "ok"},
                                 {"message", "200 OK"},
                                 {"data", std::move(objectPaths)}};
            }
            res.end();
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTreePaths", objectPath,
        static_cast<int32_t>(99), std::array<std::string, 0>());
}

void handle_enumerate(crow::Response &res, const std::string &objectPath)
{
    crow::connections::systemBus->async_method_call(
        [&res, objectPath{std::string(objectPath)}](
            const boost::system::error_code ec,
            const GetSubTreeType &object_names) {
            if (ec)
            {
                res.jsonValue = {{"message", "200 OK"},
                                 {"status", "ok"},
                                 {"data", nlohmann::json::object()}};

                res.end();
                return;
            }

            boost::container::flat_set<std::string> connections;

            for (const auto &object : object_names)
            {
                for (const auto &Connection : object.second)
                {
                    connections.insert(Connection.first);
                }
            }

            if (connections.size() <= 0)
            {
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            auto transaction =
                std::make_shared<nlohmann::json>(nlohmann::json::object());
            for (const std::string &Connection : connections)
            {
                getManagedObjectsForEnumerate(objectPath, Connection, res,
                                              transaction);
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree", objectPath,
        (int32_t)0, std::array<std::string, 0>());
}

void handle_get(crow::Response &res, std::string &objectPath,
                std::string &destProperty)
{
    BMCWEB_LOG_DEBUG << "handle_get: " << objectPath
                     << " prop:" << destProperty;
    std::shared_ptr<std::string> property_name =
        std::make_shared<std::string>(std::move(destProperty));

    std::shared_ptr<std::string> path =
        std::make_shared<std::string>(std::move(objectPath));

    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;
    crow::connections::systemBus->async_method_call(
        [&res, path, property_name](const boost::system::error_code ec,
                                    const GetObjectType &object_names) {
            if (ec || object_names.size() <= 0)
            {
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            std::shared_ptr<nlohmann::json> response =
                std::make_shared<nlohmann::json>(nlohmann::json::object());
            // The mapper should never give us an empty interface names list,
            // but check anyway
            for (const std::pair<std::string, std::vector<std::string>>
                     connection : object_names)
            {
                const std::vector<std::string> &interfaceNames =
                    connection.second;

                if (interfaceNames.size() <= 0)
                {
                    res.result(boost::beast::http::status::not_found);
                    res.end();
                    return;
                }

                for (const std::string &interface : interfaceNames)
                {
                    crow::connections::systemBus->async_method_call(
                        [&res, response, property_name](
                            const boost::system::error_code ec,
                            const std::vector<
                                std::pair<std::string, DbusRestVariantType>>
                                &properties) {
                            if (ec)
                            {
                                BMCWEB_LOG_ERROR << "Bad dbus request error: "
                                                 << ec;
                            }
                            else
                            {
                                for (const std::pair<std::string,
                                                     DbusRestVariantType>
                                         &property : properties)
                                {
                                    // if property name is empty, or matches our
                                    // search query, add it to the response json

                                    if (property_name->empty())
                                    {
                                        mapbox::util::apply_visitor(
                                            [&response, &property](auto &&val) {
                                                (*response)[property.first] =
                                                    val;
                                            },
                                            property.second);
                                    }
                                    else if (property.first == *property_name)
                                    {
                                        mapbox::util::apply_visitor(
                                            [&response](auto &&val) {
                                                (*response) = val;
                                            },
                                            property.second);
                                    }
                                }
                            }
                            if (response.use_count() == 1)
                            {
                                res.jsonValue = {{"status", "ok"},
                                                 {"message", "200 OK"},
                                                 {"data", *response}};

                                res.end();
                            }
                        },
                        connection.first, *path,
                        "org.freedesktop.DBus.Properties", "GetAll", interface);
                }
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject", *path,
        std::array<std::string, 0>());
}

struct AsyncPutRequest
{
    AsyncPutRequest(crow::Response &res) : res(res)
    {
        res.jsonValue = {
            {"status", "ok"}, {"message", "200 OK"}, {"data", nullptr}};
    }
    ~AsyncPutRequest()
    {
        if (res.result() == boost::beast::http::status::internal_server_error)
        {
            // Reset the json object to clear out any data that made it in
            // before the error happened todo(ed) handle error condition with
            // proper code
            res.jsonValue = nlohmann::json::object();
        }

        if (res.jsonValue.empty())
        {
            res.result(boost::beast::http::status::forbidden);
            res.jsonValue = {
                {"status", "error"},
                {"message", "403 Forbidden"},
                {"data",
                 {{"message", "The specified property cannot be created: " +
                                  propertyName}}}};
        }

        res.end();
    }

    void setErrorStatus()
    {
        res.result(boost::beast::http::status::internal_server_error);
    }

    crow::Response &res;
    std::string objectPath;
    std::string propertyName;
    nlohmann::json propertyValue;
};

void handlePut(const crow::Request &req, crow::Response &res,
               const std::string &objectPath, const std::string &destProperty)
{
    nlohmann::json requestDbusData =
        nlohmann::json::parse(req.body, nullptr, false);

    if (requestDbusData.is_discarded())
    {
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }

    nlohmann::json::const_iterator propertyIt = requestDbusData.find("data");
    if (propertyIt == requestDbusData.end())
    {
        res.result(boost::beast::http::status::bad_request);
        res.end();
        return;
    }
    const nlohmann::json &propertySetValue = *propertyIt;
    auto transaction = std::make_shared<AsyncPutRequest>(res);
    transaction->objectPath = objectPath;
    transaction->propertyName = destProperty;
    transaction->propertyValue = propertySetValue;

    using GetObjectType =
        std::vector<std::pair<std::string, std::vector<std::string>>>;

    crow::connections::systemBus->async_method_call(
        [transaction](const boost::system::error_code ec,
                      const GetObjectType &object_names) {
            if (!ec && object_names.size() <= 0)
            {
                transaction->res.result(boost::beast::http::status::not_found);
                return;
            }

            for (const std::pair<std::string, std::vector<std::string>>
                     connection : object_names)
            {
                const std::string &connectionName = connection.first;

                crow::connections::systemBus->async_method_call(
                    [connectionName{std::string(connectionName)},
                     transaction](const boost::system::error_code ec,
                                  const std::string &introspectXml) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec.message()
                                << " on process: " << connectionName;
                            transaction->setErrorStatus();
                            return;
                        }
                        tinyxml2::XMLDocument doc;

                        doc.Parse(introspectXml.c_str());
                        tinyxml2::XMLNode *pRoot =
                            doc.FirstChildElement("node");
                        if (pRoot == nullptr)
                        {
                            BMCWEB_LOG_ERROR << "XML document failed to parse: "
                                             << introspectXml;
                            transaction->setErrorStatus();
                            return;
                        }
                        tinyxml2::XMLElement *ifaceNode =
                            pRoot->FirstChildElement("interface");
                        while (ifaceNode != nullptr)
                        {
                            const char *interfaceName =
                                ifaceNode->Attribute("name");
                            BMCWEB_LOG_DEBUG << "found interface "
                                             << interfaceName;
                            tinyxml2::XMLElement *propNode =
                                ifaceNode->FirstChildElement("property");
                            while (propNode != nullptr)
                            {
                                const char *propertyName =
                                    propNode->Attribute("name");
                                BMCWEB_LOG_DEBUG << "Found property "
                                                 << propertyName;
                                if (propertyName == transaction->propertyName)
                                {
                                    const char *argType =
                                        propNode->Attribute("type");
                                    if (argType != nullptr)
                                    {
                                        sdbusplus::message::message m =
                                            crow::connections::systemBus
                                                ->new_method_call(
                                                    connectionName.c_str(),
                                                    transaction->objectPath
                                                        .c_str(),
                                                    "org.freedesktop.DBus."
                                                    "Properties",
                                                    "Set");
                                        m.append(interfaceName,
                                                 transaction->propertyName);
                                        int r = sd_bus_message_open_container(
                                            m.get(), SD_BUS_TYPE_VARIANT,
                                            argType);
                                        if (r < 0)
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }
                                        r = convertJsonToDbus(
                                            m.get(), argType,
                                            transaction->propertyValue);
                                        if (r < 0)
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }
                                        r = sd_bus_message_close_container(
                                            m.get());
                                        if (r < 0)
                                        {
                                            transaction->setErrorStatus();
                                            return;
                                        }

                                        crow::connections::systemBus
                                            ->async_send(
                                                m,
                                                [transaction](
                                                    boost::system::error_code
                                                        ec,
                                                    sdbusplus::message::message
                                                        &m) {
                                                    BMCWEB_LOG_DEBUG << "sent";
                                                    if (ec)
                                                    {
                                                        transaction->res
                                                            .jsonValue
                                                                ["status"] =
                                                            "error";
                                                        transaction->res
                                                            .jsonValue
                                                                ["message"] =
                                                            ec.message();
                                                    }
                                                });
                                    }
                                }
                                propNode =
                                    propNode->NextSiblingElement("property");
                            }
                            ifaceNode =
                                ifaceNode->NextSiblingElement("interface");
                        }
                    },
                    connectionName, transaction->objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
        },
        "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetObject",
        transaction->objectPath, std::array<std::string, 0>());
}

template <typename... Middlewares> void requestRoutes(Crow<Middlewares...> &app)
{
    BMCWEB_ROUTE(app, "/bus/")
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                res.jsonValue = {{"busses", {{{"name", "system"}}}},
                                 {"status", "ok"}};
            });

    BMCWEB_ROUTE(app, "/bus/system/")
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                auto myCallback = [&res](const boost::system::error_code ec,
                                         std::vector<std::string> &names) {
                    if (ec)
                    {
                        BMCWEB_LOG_ERROR << "Dbus call failed with code " << ec;
                        res.result(
                            boost::beast::http::status::internal_server_error);
                    }
                    else
                    {
                        std::sort(names.begin(), names.end());
                        nlohmann::json j{{"status", "ok"}};
                        auto &objectsSub = j["objects"];
                        for (auto &name : names)
                        {
                            objectsSub.push_back({{"name", name}});
                        }
                        res.jsonValue = std::move(j);
                    }
                    res.end();
                };
                crow::connections::systemBus->async_method_call(
                    std::move(myCallback), "org.freedesktop.DBus", "/",
                    "org.freedesktop.DBus", "ListNames");
            });

    BMCWEB_ROUTE(app, "/list/")
        .methods("GET"_method)(
            [](const crow::Request &req, crow::Response &res) {
                handle_list(res, "/");
            });

    BMCWEB_ROUTE(app, "/xyz/<path>")
        .methods("GET"_method, "PUT"_method,
                 "POST"_method)([](const crow::Request &req,
                                   crow::Response &res,
                                   const std::string &path) {
            std::string objectPath = "/xyz/" + path;

            // Trim any trailing "/" at the end
            if (boost::ends_with(objectPath, "/"))
            {
                objectPath.pop_back();
            }

            // If accessing a single attribute, fill in and update objectPath,
            // otherwise leave destProperty blank
            std::string destProperty = "";
            const char *attrSeperator = "/attr/";
            size_t attrPosition = path.find(attrSeperator);
            if (attrPosition != path.npos)
            {
                objectPath = "/xyz/" + path.substr(0, attrPosition);
                destProperty = path.substr(attrPosition + strlen(attrSeperator),
                                           path.length());
            }

            if (req.method() == "POST"_method)
            {
                constexpr const char *action_seperator = "/action/";
                size_t action_position = path.find(action_seperator);
                if (action_position != path.npos)
                {
                    objectPath = "/xyz/" + path.substr(0, action_position);
                    std::string post_property = path.substr(
                        (action_position + strlen(action_seperator)),
                        path.length());
                    handle_action(req, res, objectPath, post_property);
                    return;
                }
            }
            else if (req.method() == "GET"_method)
            {
                if (boost::ends_with(objectPath, "/enumerate"))
                {
                    objectPath.erase(objectPath.end() - 10, objectPath.end());
                    handle_enumerate(res, objectPath);
                }
                else if (boost::ends_with(objectPath, "/list"))
                {
                    objectPath.erase(objectPath.end() - 5, objectPath.end());
                    handle_list(res, objectPath);
                }
                else
                {
                    handle_get(res, objectPath, destProperty);
                }
                return;
            }
            else if (req.method() == "PUT"_method)
            {
                handlePut(req, res, objectPath, destProperty);
                return;
            }

            res.result(boost::beast::http::status::method_not_allowed);
            res.end();
        });

    BMCWEB_ROUTE(app, "/bus/system/<str>/")
        .methods("GET"_method)([](const crow::Request &req, crow::Response &res,
                                  const std::string &Connection) {
            std::shared_ptr<nlohmann::json> transaction;
            introspectObjects(res, Connection, "/", transaction);
        });

    BMCWEB_ROUTE(app, "/download/dump/<str>/")
        .methods("GET"_method)([](const crow::Request &req, crow::Response &res,
                                  const std::string &dumpId) {
            std::regex validFilename("^[\\w\\- ]+(\\.?[\\w\\- ]+)$");
            if (!std::regex_match(dumpId, validFilename))
            {
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            std::experimental::filesystem::path loc(
                "/var/lib/phosphor-debug-collector/dumps");

            loc += dumpId;

            if (!std::experimental::filesystem::exists(loc) ||
                !std::experimental::filesystem::is_directory(loc))
            {
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            std::experimental::filesystem::directory_iterator files(loc);
            for (auto &file : files)
            {
                std::ifstream readFile(file.path());
                if (readFile.good())
                {
                    continue;
                }
                res.addHeader("Content-Type", "application/octet-stream");
                res.body() = {std::istreambuf_iterator<char>(readFile),
                              std::istreambuf_iterator<char>()};
                res.end();
            }
            res.result(boost::beast::http::status::not_found);
            res.end();
            return;
        });

    BMCWEB_ROUTE(app, "/bus/system/<str>/<path>")
        .methods("GET"_method)([](const crow::Request &req, crow::Response &res,
                                  const std::string &processName,
                                  const std::string &requestedPath) {
            std::vector<std::string> strs;
            boost::split(strs, requestedPath, boost::is_any_of("/"));
            std::string objectPath;
            std::string interfaceName;
            std::string methodName;
            auto it = strs.begin();
            if (it == strs.end())
            {
                objectPath = "/";
            }
            while (it != strs.end())
            {
                // Check if segment contains ".".  If it does, it must be an
                // interface
                if (it->find(".") != std::string::npos)
                {
                    break;
                    // THis check is neccesary as the trailing slash gets parsed
                    // as part of our <path> specifier above, which causes the
                    // normal trailing backslash redirector to fail.
                }
                else if (!it->empty())
                {
                    objectPath += "/" + *it;
                }
                it++;
            }
            if (it != strs.end())
            {
                interfaceName = *it;
                it++;

                // after interface, we might have a method name
                if (it != strs.end())
                {
                    methodName = *it;
                    it++;
                }
            }
            if (it != strs.end())
            {
                // if there is more levels past the method name, something went
                // wrong, return not found
                res.result(boost::beast::http::status::not_found);
                res.end();
                return;
            }
            if (interfaceName.empty())
            {
                crow::connections::systemBus->async_method_call(
                    [&, processName,
                     objectPath](const boost::system::error_code ec,
                                 const std::string &introspect_xml) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec.message()
                                << " on process: " << processName
                                << " path: " << objectPath << "\n";
                        }
                        else
                        {
                            tinyxml2::XMLDocument doc;

                            doc.Parse(introspect_xml.c_str());
                            tinyxml2::XMLNode *pRoot =
                                doc.FirstChildElement("node");
                            if (pRoot == nullptr)
                            {
                                BMCWEB_LOG_ERROR
                                    << "XML document failed to parse "
                                    << processName << " " << objectPath << "\n";
                                res.jsonValue = {{"status", "XML parse error"}};
                                res.result(boost::beast::http::status::
                                               internal_server_error);
                            }
                            else
                            {
                                nlohmann::json interfacesArray =
                                    nlohmann::json::array();
                                tinyxml2::XMLElement *interface =
                                    pRoot->FirstChildElement("interface");

                                while (interface != nullptr)
                                {
                                    std::string ifaceName =
                                        interface->Attribute("name");
                                    interfacesArray.push_back(
                                        {{"name", ifaceName}});

                                    interface = interface->NextSiblingElement(
                                        "interface");
                                }
                                res.jsonValue = {
                                    {"status", "ok"},
                                    {"bus_name", processName},
                                    {"interfaces", interfacesArray},
                                    {"objectPath", objectPath}};
                            }
                        }
                        res.end();
                    },
                    processName, objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
            else
            {
                crow::connections::systemBus->async_method_call(
                    [&, processName, objectPath,
                     interface_name{std::move(interfaceName)}](
                        const boost::system::error_code ec,
                        const std::string &introspect_xml) {
                        if (ec)
                        {
                            BMCWEB_LOG_ERROR
                                << "Introspect call failed with error: "
                                << ec.message()
                                << " on process: " << processName
                                << " path: " << objectPath << "\n";
                        }
                        else
                        {
                            tinyxml2::XMLDocument doc;

                            doc.Parse(introspect_xml.c_str());
                            tinyxml2::XMLNode *pRoot =
                                doc.FirstChildElement("node");
                            if (pRoot == nullptr)
                            {
                                BMCWEB_LOG_ERROR
                                    << "XML document failed to parse "
                                    << processName << " " << objectPath << "\n";
                                res.result(boost::beast::http::status::
                                               internal_server_error);
                            }
                            else
                            {
                                tinyxml2::XMLElement *node =
                                    pRoot->FirstChildElement("node");

                                // if we know we're the only call, build the
                                // json directly
                                nlohmann::json methodsArray =
                                    nlohmann::json::array();
                                nlohmann::json signalsArray =
                                    nlohmann::json::array();
                                tinyxml2::XMLElement *interface =
                                    pRoot->FirstChildElement("interface");

                                while (interface != nullptr)
                                {
                                    std::string ifaceName =
                                        interface->Attribute("name");

                                    if (ifaceName == interfaceName)
                                    {
                                        tinyxml2::XMLElement *methods =
                                            interface->FirstChildElement(
                                                "method");
                                        while (methods != nullptr)
                                        {
                                            nlohmann::json argsArray =
                                                nlohmann::json::array();
                                            tinyxml2::XMLElement *arg =
                                                methods->FirstChildElement(
                                                    "arg");
                                            while (arg != nullptr)
                                            {
                                                argsArray.push_back(
                                                    {{"name",
                                                      arg->Attribute("name")},
                                                     {"type",
                                                      arg->Attribute("type")},
                                                     {"direction",
                                                      arg->Attribute(
                                                          "direction")}});
                                                arg = arg->NextSiblingElement(
                                                    "arg");
                                            }
                                            methodsArray.push_back(
                                                {{"name",
                                                  methods->Attribute("name")},
                                                 {"uri",
                                                  "/bus/system/" + processName +
                                                      objectPath + "/" +
                                                      interfaceName + "/" +
                                                      methods->Attribute(
                                                          "name")},
                                                 {"args", argsArray}});
                                            methods =
                                                methods->NextSiblingElement(
                                                    "method");
                                        }
                                        tinyxml2::XMLElement *signals =
                                            interface->FirstChildElement(
                                                "signal");
                                        while (signals != nullptr)
                                        {
                                            nlohmann::json argsArray =
                                                nlohmann::json::array();

                                            tinyxml2::XMLElement *arg =
                                                signals->FirstChildElement(
                                                    "arg");
                                            while (arg != nullptr)
                                            {
                                                std::string name =
                                                    arg->Attribute("name");
                                                std::string type =
                                                    arg->Attribute("type");
                                                argsArray.push_back({
                                                    {"name", name},
                                                    {"type", type},
                                                });
                                                arg = arg->NextSiblingElement(
                                                    "arg");
                                            }
                                            signalsArray.push_back(
                                                {{"name",
                                                  signals->Attribute("name")},
                                                 {"args", argsArray}});
                                            signals =
                                                signals->NextSiblingElement(
                                                    "signal");
                                        }

                                        res.jsonValue = {
                                            {"status", "ok"},
                                            {"bus_name", processName},
                                            {"interface", interfaceName},
                                            {"methods", methodsArray},
                                            {"objectPath", objectPath},
                                            {"properties",
                                             nlohmann::json::object()},
                                            {"signals", signalsArray}};

                                        break;
                                    }

                                    interface = interface->NextSiblingElement(
                                        "interface");
                                }
                                if (interface == nullptr)
                                {
                                    // if we got to the end of the list and
                                    // never found a match, throw 404
                                    res.result(
                                        boost::beast::http::status::not_found);
                                }
                            }
                        }
                        res.end();
                    },
                    processName, objectPath,
                    "org.freedesktop.DBus.Introspectable", "Introspect");
            }
        });
}
} // namespace openbmc_mapper
} // namespace crow
