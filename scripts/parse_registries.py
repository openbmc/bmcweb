#!/usr/bin/env python3
import argparse
import json
import os
from collections import OrderedDict

import requests

PRAGMA_ONCE = """#pragma once
"""

WARNING = """/****************************************************************
 *                 READ THIS WARNING FIRST
 * This is an auto-generated header which contains definitions
 * for Redfish DMTF defined messages.
 * DO NOT modify this registry outside of running the
 * parse_registries.py script.  The definitions contained within
 * this file are owned by DMTF.  Any modifications to these files
 * should be first pushed to the relevant registry in the DMTF
 * github organization.
 ***************************************************************/"""

REGISTRY_HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries::{}
{{
"""
)

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

include_path = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "registries")
)

proxies = {"https": os.environ.get("https_proxy", None)}


def make_getter(dmtf_name, header_name, type_name):
    url = "https://redfish.dmtf.org/registries/{}".format(dmtf_name)
    dmtf = requests.get(url, proxies=proxies)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text, object_pairs_hook=OrderedDict)
    path = os.path.join(include_path, header_name)
    return (path, json_file, type_name, url)


def openbmc_local_getter():
    url = ""
    with open(
        os.path.join(
            SCRIPT_DIR,
            "..",
            "redfish-core",
            "include",
            "registries",
            "openbmc.json",
        ),
        "rb",
    ) as json_file:
        json_file = json.load(json_file)

    path = os.path.join(include_path, "openbmc_message_registry.hpp")
    return (path, json_file, "openbmc", url)


def update_registries(files):
    # Remove the old files
    for file, json_dict, namespace, url in files:
        try:
            os.remove(file)
        except BaseException:
            print("{} not found".format(file))

        with open(file, "w") as registry:
            registry.write(REGISTRY_HEADER.format(namespace))
            # Parse the Registry header info
            registry.write(
                "const Header header = {{\n"
                '    "{json_dict[@Redfish.Copyright]}",\n'
                '    "{json_dict[@odata.type]}",\n'
                '    "{json_dict[Id]}",\n'
                '    "{json_dict[Name]}",\n'
                '    "{json_dict[Language]}",\n'
                '    "{json_dict[Description]}",\n'
                '    "{json_dict[RegistryPrefix]}",\n'
                '    "{json_dict[RegistryVersion]}",\n'
                '    "{json_dict[OwningEntity]}",\n'
                "}};\n"
                "constexpr const char* url =\n"
                '    "{url}";\n'
                "\n"
                "constexpr std::array registry =\n"
                "{{\n".format(
                    json_dict=json_dict,
                    url=url,
                )
            )

            messages_sorted = sorted(json_dict["Messages"].items())
            for messageId, message in messages_sorted:
                registry.write(
                    "    MessageEntry{{\n"
                    '        "{messageId}",\n'
                    "        {{\n"
                    '            "{message[Description]}",\n'
                    '            "{message[Message]}",\n'
                    '            "{message[MessageSeverity]}",\n'
                    "            {message[NumberOfArgs]},\n"
                    "            {{".format(
                        messageId=messageId, message=message
                    )
                )
                paramTypes = message.get("ParamTypes")
                if paramTypes:
                    for paramType in paramTypes:
                        registry.write(
                            '\n                "{}",'.format(paramType)
                        )
                    registry.write("\n            },\n")
                else:
                    registry.write("},\n")
                registry.write(
                    '            "{message[Resolution]}",\n'
                    "        }}}},\n".format(message=message)
                )

            registry.write("\n};\n\nenum class Index\n{\n")
            for index, (messageId, message) in enumerate(messages_sorted):
                messageId = messageId[0].lower() + messageId[1:]
                registry.write("    {} = {},\n".format(messageId, index))
            registry.write(
                "}};\n}} // namespace redfish::registries::{}\n".format(
                    namespace
                )
            )


def get_privilege_string_from_list(privilege_list):
    privilege_string = "{{\n"
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        privilege_string += "    {"
        for privilege in privileges:
            if privilege == "NoAuth":
                continue
            privilege_string += '"'
            privilege_string += privilege
            privilege_string += '",\n'
        if privilege != "NoAuth":
            privilege_string = privilege_string[:-2]
        privilege_string += "}"
        privilege_string += ",\n"
    privilege_string = privilege_string[:-2]
    privilege_string += "\n}}"
    return privilege_string


def get_variable_name_for_privilege_set(privilege_list):
    names = []
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        names.append("And".join(privileges))
    return "Or".join(names)


PRIVILEGE_HEADER = (
    PRAGMA_ONCE
    + WARNING
    + """
#include "privileges.hpp"

#include <array>

// clang-format off

namespace redfish::privileges
{
"""
)


def get_response_code(entry_id, entry):
    codes = {
        "InternalError": "internal_server_error",
        "OperationTimeout": "internal_server_error",
        "PropertyValueResourceConflict": "conflict",
        "ResourceInUse": "service_unavailable",
        "ServiceTemporarilyUnavailable": "service_unavailable",
        "ResourceCannotBeDeleted": "method_not_allowed",
        "PropertyValueModified": "ok",
        "InsufficientPrivilege": "forbidden",
        "AccountForSessionNoLongerExists": "forbidden",
        "ServiceDisabled": "service_unavailable",
        "ServiceInUnknownState": "service_unavailable",
        "EventSubscriptionLimitExceeded": "service_unavailable",
        "ResourceAtUriUnauthorized": "unauthorized",
        "SessionTerminated": "ok",
        "SubscriptionTerminated": "ok",
        "PropertyNotWritable": "forbidden",
        "MaximumErrorsExceeded": "internal_server_error",
        "GeneralError": "internal_server_error",
        "PreconditionFailed": "precondition_failed",
        "OperationFailed": "bad_gateway",
        "ServiceShuttingDown": "service_unavailable",
        "AccountRemoved": "ok",
        "PropertyValueExternalConflict": "conflict",
        "InsufficientStorage": "insufficient_storage",
        "OperationNotAllowed": "method_not_allowed",
        "ResourceNotFound": "not_found",
        "CouldNotEstablishConnection": "not_found",
        "AccessDenied": "forbidden",
        "Success": None,
        "Created": "created",
        "NoValidSession": "forbidden",
        "SessionLimitExceeded": "service_unavailable",
        "ResourceExhaustion": "service_unavailable",
        "AccountModified": "ok",
        "PasswordChangeRequired": None,
        "ResourceInStandby": "service_unavailable",
        "GenerateSecretKeyRequired": "forbidden",
    }

    code = codes.get(entry_id, "NOCODE")
    if code != "NOCODE":
        return code

    return "bad_request"


def get_old_index(entry):
    old_order = [
        "ResourceInUse",
        "MalformedJSON",
        "ResourceMissingAtURI",
        "ActionParameterValueFormatError",
        "ActionParameterValueNotInList",
        "InternalError",
        "UnrecognizedRequestBody",
        "ResourceAtUriUnauthorized",
        "ActionParameterUnknown",
        "ResourceCannotBeDeleted",
        "PropertyDuplicate",
        "ServiceTemporarilyUnavailable",
        "ResourceAlreadyExists",
        "AccountForSessionNoLongerExists",
        "CreateFailedMissingReqProperties",
        "PropertyValueFormatError",
        "PropertyValueNotInList",
        "PropertyValueOutOfRange",
        "ResourceAtUriInUnknownFormat",
        "ServiceDisabled",
        "ServiceInUnknownState",
        "EventSubscriptionLimitExceeded",
        "ActionParameterMissing",
        "StringValueTooLong",
        "SessionTerminated",
        "SubscriptionTerminated",
        "ResourceTypeIncompatible",
        "ResetRequired",
        "ChassisPowerStateOnRequired",
        "ChassisPowerStateOffRequired",
        "PropertyValueConflict",
        "PropertyValueResourceConflict",
        "PropertyValueExternalConflict",
        "PropertyValueIncorrect",
        "ResourceCreationConflict",
        "MaximumErrorsExceeded",
        "PreconditionFailed",
        "PreconditionRequired",
        "OperationFailed",
        "OperationTimeout",
        "PropertyValueTypeError",
        "PropertyValueError",
        "ResourceNotFound",
        "CouldNotEstablishConnection",
        "PropertyNotWritable",
        "QueryParameterValueTypeError",
        "ServiceShuttingDown",
        "ActionParameterDuplicate",
        "ActionParameterNotSupported",
        "SourceDoesNotSupportProtocol",
        "StrictAccountTypes",
        "AccountRemoved",
        "AccessDenied",
        "QueryNotSupported",
        "CreateLimitReachedForResource",
        "GeneralError",
        "Success",
        "Created",
        "NoOperation",
        "PropertyUnknown",
        "NoValidSession",
        "InvalidObject",
        "ResourceInStandby",
        "ActionParameterValueTypeError",
        "ActionParameterValueError",
        "SessionLimitExceeded",
        "ActionNotSupported",
        "InvalidIndex",
        "EmptyJSON",
        "QueryNotSupportedOnResource",
        "QueryNotSupportedOnOperation",
        "QueryCombinationInvalid",
        "EventBufferExceeded",
        "InsufficientPrivilege",
        "PropertyValueModified",
        "AccountNotModified",
        "QueryParameterValueFormatError",
        "PropertyMissing",
        "ResourceExhaustion",
        "AccountModified",
        "QueryParameterOutOfRange",
        "PasswordChangeRequired",
        "InsufficientStorage",
        "OperationNotAllowed",
        "ArraySizeTooLong",
        "Invalid File",
        "GenerateSecretKeyRequired",
    ]

    if entry[0] in old_order:
        return old_order.index(entry[0])
    else:
        return 999999


def make_error_function(entry_id, entry, is_header):

    arg_as_url = {
        "AccessDenied": [1],
        "CouldNotEstablishConnection": [1],
        "GenerateSecretKeyRequired": [1],
        "InvalidObject": [1],
        "PasswordChangeRequired": [1],
        "PropertyValueResourceConflict": [3],
        "ResetRequired": [1],
        "ResourceAtUriInUnknownFormat": [1],
        "ResourceAtUriUnauthorized": [1],
        "ResourceCreationConflict": [1],
        "ResourceMissingAtURI": [1],
        "SourceDoesNotSupportProtocol": [1],
    }

    arg_as_json = {
        "ActionParameterValueError": [1],
        "ActionParameterValueFormatError": [1],
        "ActionParameterValueTypeError": [1],
        "PropertyValueExternalConflict": [2],
        "PropertyValueFormatError": [1],
        "PropertyValueIncorrect": [2],
        "PropertyValueModified": [2],
        "PropertyValueNotInList": [1],
        "PropertyValueOutOfRange": [1],
        "PropertyValueResourceConflict": [2],
        "PropertyValueTypeError": [1],
        "QueryParameterValueFormatError": [1],
        "QueryParameterValueTypeError": [1],
    }

    arg_as_int = {
        "StringValueTooLong": [2],
    }

    arg_as_uint64 = {
        "ArraySizeTooLong": [2],
    }
    arg_as_int64 = {
        "InvalidIndex": [1],
    }

    out = ""
    args = []
    argtypes = []
    for arg_index, arg in enumerate(entry.get("ParamTypes", [])):
        arg_index += 1
        if arg_index in arg_as_url.get(entry_id, []):
            typename = "const boost::urls::url_view_base&"
        elif arg_index in arg_as_json.get(entry_id, []):
            typename = "const nlohmann::json&"
        elif arg_index in arg_as_int.get(entry_id, []):
            typename = "int"
        elif arg_index in arg_as_uint64.get(entry_id, []):
            typename = "uint64_t"
        elif arg_index in arg_as_int64.get(entry_id, []):
            typename = "int64_t"
        else:
            typename = "std::string_view"
        argtypes.append(typename)
        args.append(f"{typename} arg{arg_index}")
    function_name = entry_id[0].lower() + entry_id[1:]
    arg = ", ".join(args)
    out += f"nlohmann::json {function_name}({arg})"

    if is_header:
        out += ";\n\n"
    else:
        out += "\n{\n"
        to_array_type = ""
        if argtypes:
            outargs = []
            for index, typename in enumerate(argtypes):
                index += 1
                if typename == "const nlohmann::json&":
                    out += f"std::string arg{index}Str = arg{index}.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);\n"
                elif typename in ("int64_t", "int", "uint64_t"):
                    out += f"std::string arg{index}Str = std::to_string(arg{index});\n"

            for index, typename in enumerate(argtypes):
                index += 1
                if typename == "const boost::urls::url_view_base&":
                    outargs.append(f"arg{index}.buffer()")
                    to_array_type = "<std::string_view>"
                elif typename == "const nlohmann::json&":
                    outargs.append(f"arg{index}Str")
                    to_array_type = "<std::string_view>"
                elif typename in ("int64_t", "int", "uint64_t"):
                    outargs.append(f"arg{index}Str")
                    to_array_type = "<std::string_view>"
                else:
                    outargs.append(f"arg{index}")
            argstring = ", ".join(outargs)
            # out += f"    std::array<std::string_view, {len(argtypes)}> args{{{argstring}}};\n"

        if argtypes:
            arg_param = f"std::to_array{to_array_type}({{{argstring}}})"
        else:
            arg_param = "{}"
        out += f"    return getLog(redfish::registries::base::Index::{function_name}, {arg_param});"
        out += "\n}\n\n"
    args.insert(0, "crow::Response& res")
    if entry_id == "InternalError":
        if is_header:
            args.append(
                "std::source_location location = std::source_location::current()"
            )
        else:
            args.append("const std::source_location location")
    arg = ", ".join(args)
    out += f"void {function_name}({arg})"
    if is_header:
        out += ";\n"
    else:
        out += "\n{\n"
        if entry_id == "InternalError":
            out += """BMCWEB_LOG_CRITICAL("Internal Error {}({}:{}) `{}`: ", location.file_name(),
                        location.line(), location.column(),
                        location.function_name());\n"""

        if entry_id == "ServiceTemporarilyUnavailable":
            out += (
                "res.addHeader(boost::beast::http::field::retry_after, arg1);"
            )

        res = get_response_code(entry_id, entry)
        if res:
            out += f"    res.result(boost::beast::http::status::{res});\n"
        args_out = ", ".join([f"arg{x+1}" for x in range(len(argtypes))])

        addMessageToJson = {
            "PropertyDuplicate": 1,
            "ResourceAlreadyExists": 2,
            "CreateFailedMissingReqProperties": 1,
            "PropertyValueFormatError": 2,
            "PropertyValueNotInList": 2,
            "PropertyValueTypeError": 2,
            "PropertyValueError": 1,
            "PropertyNotWritable": 1,
            "PropertyValueModified": 1,
            "PropertyMissing": 1,
        }

        addMessageToRoot = [
            "SessionTerminated",
            "SubscriptionTerminated",
            "AccountRemoved",
            "Created",
            "Success",
            "PasswordChangeRequired",
        ]

        if entry_id in addMessageToJson:
            out += f"    addMessageToJson(res.jsonValue, {function_name}({args_out}), arg{addMessageToJson[entry_id]});\n"
        elif entry_id in addMessageToRoot:
            out += f"    addMessageToJsonRoot(res.jsonValue, {function_name}({args_out}));\n"
        else:
            out += f"    addMessageToErrorJson(res.jsonValue, {function_name}({args_out}));\n"
        out += "}\n"
    out += "\n"
    return out


def create_error_registry(entry):
    file, json_dict, namespace, url = entry

    messages = OrderedDict(
        sorted(json_dict["Messages"].items(), key=get_old_index)
    )
    error_messages_hpp = os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "include", "error_messages.hpp"
    )
    with open(
        error_messages_hpp,
        "w",
    ) as out:
        out.write(PRAGMA_ONCE)
        out.write(WARNING)
        out.write(
            """

#include "http_response.hpp"

#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <source_location>
#include <string>
#include <string_view>
#include <utility>

// IWYU pragma: no_forward_declare crow::Response

namespace redfish
{

namespace messages
{

    constexpr const char* messageVersionPrefix = "Base.1.11.0.";
    constexpr const char* messageAnnotation = "@Message.ExtendedInfo";

    /**
    * @brief Moves all error messages from the |source| JSON to |target|
    */
    void moveErrorsToErrorJson(nlohmann::json& target, nlohmann::json& source);

"""
        )

        for entry_id, entry in messages.items():
            message = entry["Message"]
            for index in range(1, 10):
                message = message.replace(f"'%{index}'", f"<arg{index}>")
                message = message.replace(f"%{index}", f"<arg{index}>")

            out.write("/**\n")
            out.write(f"* @brief Formats {entry_id} message into JSON\n")
            out.write(f'* Message body: "{message}"\n')
            out.write("*\n")
            arg_index = 0
            for arg_index, arg in enumerate(entry.get("ParamTypes", [])):
                arg_index += 1

                out.write(
                    f"* @param[in] arg{arg_index} Parameter of message that will replace %{arg_index} in its body.\n"
                )
            out.write("*\n")
            out.write(f"* @returns Message {entry_id} formatted to JSON */\n")

            out.write(make_error_function(entry_id, entry, True))
        out.write("    }\n")
        out.write("}\n")

    error_messages_cpp = os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "src", "error_messages.cpp"
    )
    with open(
        error_messages_cpp,
        "w",
    ) as out:
        out.write(WARNING)
        out.write(
            """
#include "error_messages.hpp"

#include "http_response.hpp"
#include "logging.hpp"
#include "registries.hpp"
#include "registries/base_message_registry.hpp"

#include <boost/beast/http/field.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <source_location>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace redfish
{

namespace messages
{

static void addMessageToErrorJson(nlohmann::json& target,
                                  const nlohmann::json& message)
{
    auto& error = target["error"];

    // If this is the first error message, fill in the information from the
    // first error message to the top level struct
    if (!error.is_object())
    {
        auto messageIdIterator = message.find("MessageId");
        if (messageIdIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL(
                "Attempt to add error message without MessageId");
            return;
        }

        auto messageFieldIterator = message.find("Message");
        if (messageFieldIterator == message.end())
        {
            BMCWEB_LOG_CRITICAL("Attempt to add error message without Message");
            return;
        }
        error["code"] = *messageIdIterator;
        error["message"] = *messageFieldIterator;
    }
    else
    {
        // More than 1 error occurred, so the message has to be generic
        error["code"] = std::string(messageVersionPrefix) + "GeneralError";
        error["message"] = "A general error has occurred. See Resolution for "
                           "information on how to resolve the error.";
    }

    // This check could technically be done in the default construction
    // branch above, but because we need the pointer to the extended info field
    // anyway, it's more efficient to do it here.
    auto& extendedInfo = error[messages::messageAnnotation];
    if (!extendedInfo.is_array())
    {
        extendedInfo = nlohmann::json::array();
    }

    extendedInfo.push_back(message);
}

void moveErrorsToErrorJson(nlohmann::json& target, nlohmann::json& source)
{
    if (!source.is_object())
    {
        return;
    }
    auto errorIt = source.find("error");
    if (errorIt == source.end())
    {
        // caller puts error message in root
        messages::addMessageToErrorJson(target, source);
        source.clear();
        return;
    }
    auto extendedInfoIt = errorIt->find(messages::messageAnnotation);
    if (extendedInfoIt == errorIt->end())
    {
        return;
    }
    const nlohmann::json::array_t* extendedInfo =
        (*extendedInfoIt).get_ptr<const nlohmann::json::array_t*>();
    if (extendedInfo == nullptr)
    {
        source.erase(errorIt);
        return;
    }
    for (const nlohmann::json& message : *extendedInfo)
    {
        addMessageToErrorJson(target, message);
    }
    source.erase(errorIt);
}

static void addMessageToJsonRoot(nlohmann::json& target,
                                 const nlohmann::json& message)
{
    if (!target[messages::messageAnnotation].is_array())
    {
        // Force object to be an array
        target[messages::messageAnnotation] = nlohmann::json::array();
    }

    target[messages::messageAnnotation].push_back(message);
}

static void addMessageToJson(nlohmann::json& target,
                             const nlohmann::json& message,
                             std::string_view fieldPath)
{
    std::string extendedInfo(fieldPath);
    extendedInfo += messages::messageAnnotation;

    nlohmann::json& field = target[extendedInfo];
    if (!field.is_array())
    {
        // Force object to be an array
        field = nlohmann::json::array();
    }

    // Object exists and it is an array so we can just push in the message
    field.push_back(message);
}

static nlohmann::json getLog(redfish::registries::base::Index name,
                             std::span<const std::string_view> args)
{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::base::registry.size())
    {
        return {};
    }
    return getLogFromRegistry(redfish::registries::base::header,
                              redfish::registries::base::registry, index, args);
}

"""
        )
        for entry_id, entry in messages.items():
            out.write(
                f"""/**
 * @internal
 * @brief Formats {entry_id} message into JSON
 *
 * See header file for more information
 * @endinternal
 */
"""
            )
            message = entry["Message"]
            out.write(make_error_function(entry_id, entry, False))

        out.write("    }\n")
        out.write("}\n")
    os.system(f"clang-format -i {error_messages_hpp} {error_messages_cpp}")


def make_privilege_registry():
    path, json_file, type_name, url = make_getter(
        "Redfish_1.5.0_PrivilegeRegistry.json",
        "privilege_registry.hpp",
        "privilege",
    )
    with open(path, "w") as registry:
        registry.write(PRIVILEGE_HEADER)

        privilege_dict = {}
        for mapping in json_file["Mappings"]:
            # first pass, identify all the unique privilege sets
            for operation, privilege_list in mapping["OperationMap"].items():
                privilege_dict[
                    get_privilege_string_from_list(privilege_list)
                ] = (privilege_list,)
        for index, key in enumerate(privilege_dict):
            (privilege_list,) = privilege_dict[key]
            name = get_variable_name_for_privilege_set(privilege_list)
            registry.write(
                "const std::array<Privileges, {length}> "
                "privilegeSet{name} = {key};\n".format(
                    length=len(privilege_list), name=name, key=key
                )
            )
            privilege_dict[key] = (privilege_list, name)

        for mapping in json_file["Mappings"]:
            entity = mapping["Entity"]
            registry.write("// {}\n".format(entity))
            for operation, privilege_list in mapping["OperationMap"].items():
                privilege_string = get_privilege_string_from_list(
                    privilege_list
                )
                operation = operation.lower()

                registry.write(
                    "const static auto& {}{} = privilegeSet{};\n".format(
                        operation, entity, privilege_dict[privilege_string][1]
                    )
                )
            registry.write("\n")
        registry.write(
            "} // namespace redfish::privileges\n// clang-format on\n"
        )


def to_pascal_case(text):
    s = text.replace("_", " ")
    s = s.split()
    if len(text) == 0:
        return text
    return "".join(i.capitalize() for i in s[0:])


def main():
    dmtf_registries = (
        ("base", "1.19.0"),
        ("composition", "1.1.2"),
        ("environmental", "1.0.1"),
        ("ethernet_fabric", "1.0.1"),
        ("fabric", "1.0.2"),
        ("heartbeat_event", "1.0.1"),
        ("job_event", "1.0.1"),
        ("license", "1.0.3"),
        ("log_service", "1.0.1"),
        ("network_device", "1.0.3"),
        ("platform", "1.0.1"),
        ("power", "1.0.1"),
        ("resource_event", "1.3.0"),
        ("sensor_event", "1.0.1"),
        ("storage_device", "1.2.1"),
        ("task_event", "1.0.3"),
        ("telemetry", "1.0.0"),
        ("update", "1.0.2"),
    )

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--registries",
        type=str,
        default="privilege,openbmc,"
        + ",".join([dmtf[0] for dmtf in dmtf_registries]),
        help="Comma delimited list of registries to update",
    )

    args = parser.parse_args()

    registries = set(args.registries.split(","))
    files = []

    for registry, version in dmtf_registries:
        if registry in registries:
            registry_pascal_case = to_pascal_case(registry)
            files.append(
                make_getter(
                    f"{registry_pascal_case}.{version}.json",
                    f"{registry}_message_registry.hpp",
                    registry,
                )
            )
    if "openbmc" in registries:
        files.append(openbmc_local_getter())

    update_registries(files)

    create_error_registry(files[0])

    if "privilege" in registries:
        make_privilege_registry()


if __name__ == "__main__":
    main()
