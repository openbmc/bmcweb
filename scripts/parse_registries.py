#!/usr/bin/env python3
import argparse
import json
import os
import typing as t
from collections import OrderedDict

import requests

PRAGMA_ONCE: t.Final[
    str
] = """#pragma once
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

COPYRIGHT: t.Final[
    str
] = """// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright OpenBMC Authors
"""

INCLUDES = """
#include "registries.hpp"

#include <array>

// clang-format off

namespace redfish::registries
{{
struct {}
{{
"""

REGISTRY_HEADER: t.Final[str] = f"{COPYRIGHT}{PRAGMA_ONCE}{WARNING}{INCLUDES}"

SCRIPT_DIR: t.Final[str] = os.path.dirname(os.path.realpath(__file__))

INCLUDE_PATH: t.Final[str] = os.path.realpath(
    os.path.join(SCRIPT_DIR, "..", "redfish-core", "include", "registries")
)

PROXIES: t.Final[t.Dict[str, str]] = {
    "https": os.environ.get("https_proxy", "")
}

RegistryInfo: t.TypeAlias = t.Tuple[str, t.Dict[str, t.Any], str, str]


def make_getter(
    dmtf_name: str, header_name: str, type_name: str
) -> RegistryInfo:
    url = "https://redfish.dmtf.org/registries/{}".format(dmtf_name)
    dmtf = requests.get(url, proxies=PROXIES)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text, object_pairs_hook=OrderedDict)
    path = os.path.join(INCLUDE_PATH, header_name)
    return (path, json_file, type_name, url)


def openbmc_local_getter() -> RegistryInfo:
    url = "https://raw.githubusercontent.com/openbmc/bmcweb/refs/heads/master/redfish-core/include/registries/openbmc.json"
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
    ) as json_file_fd:
        json_file = json.load(json_file_fd)

    path = os.path.join(INCLUDE_PATH, "openbmc_message_registry.hpp")
    return (path, json_file, "openbmc", url)


def update_registries(files: t.List[RegistryInfo]) -> None:
    # Remove the old files
    for file, json_dict, namespace, url in files:
        try:
            os.remove(file)
        except BaseException:
            print("{} not found".format(file))

        with open(file, "w") as registry:
            version_split = json_dict["RegistryVersion"].split(".")

            struct_name = to_pascal_case(namespace)
            registry.write(REGISTRY_HEADER.format(struct_name))
            # Parse the Registry header info
            description = json_dict.get("Description", "")
            registry.write(
                "static constexpr Header header = {{\n"
                '    "{json_dict[@Redfish.Copyright]}",\n'
                '    "{json_dict[@odata.type]}",\n'
                "    {version_split[0]},\n"
                "    {version_split[1]},\n"
                "    {version_split[2]},\n"
                '    "{json_dict[Name]}",\n'
                '    "{json_dict[Language]}",\n'
                '    "{description}",\n'
                '    "{json_dict[RegistryPrefix]}",\n'
                '    "{json_dict[OwningEntity]}",\n'
                "}};\n"
                "\n"
                "static constexpr const char* url =\n"
                '    "{url}";\n'
                "\n"
                "static constexpr std::array registry =\n"
                "{{\n".format(
                    json_dict=json_dict,
                    url=url,
                    version_split=version_split,
                    description=description,
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
                    '            "{message[Resolution]}",\n        }}}},\n'.format(
                        message=message
                    )
                )

            registry.write("\n};\n\nenum class Index\n{\n")
            for index, (messageId, message) in enumerate(messages_sorted):
                messageId = messageId[0].lower() + messageId[1:]
                registry.write("    {} = {},\n".format(messageId, index))
            registry.write("};\n")
            registry.write("}}; // struct {}\n".format(namespace))
            registry.write("\n")
            registry.write(
                "[[gnu::constructor]] inline void register{}()\n".format(
                    to_pascal_case(namespace)
                )
            )
            registry.write(
                "{{ registerRegistry<{}>(); }}\n".format(
                    to_pascal_case(namespace)
                )
            )
            registry.write("\n")
            registry.write("} // namespace redfish::registries\n")


def get_privilege_string_from_list(
    privilege_list: t.List[t.Dict[str, t.Any]],
) -> str:
    privilege_string = "{{\n"
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        privilege_string += "    {"
        last_privelege = ""
        for privilege in privileges:
            last_privelege = privilege
            if privilege == "NoAuth":
                continue
            privilege_string += '"'
            privilege_string += privilege
            privilege_string += '",\n'
        if last_privelege != "NoAuth":
            privilege_string = privilege_string[:-2]
        privilege_string += "}"
        privilege_string += ",\n"
    privilege_string = privilege_string[:-2]
    privilege_string += "\n}}"
    return privilege_string


def get_variable_name_for_privilege_set(
    privilege_list: t.List[t.Dict[str, t.Any]],
) -> str:
    names = []
    for privilege_json in privilege_list:
        privileges = privilege_json["Privilege"]
        names.append("And".join(privileges))
    return "Or".join(names)


PRIVILEGE_HEADER = (
    COPYRIGHT
    + PRAGMA_ONCE
    + WARNING
    + """
#include "privileges.hpp"

#include <array>

// clang-format off

namespace redfish::privileges
{
"""
)


def get_response_code(entry_id: str) -> str | None:
    codes = {
        "AccessDenied": "forbidden",
        "AccountForSessionNoLongerExists": "forbidden",
        "AccountModified": "ok",
        "AccountRemoved": "ok",
        "CouldNotEstablishConnection": "not_found",
        "Created": "created",
        "EventSubscriptionLimitExceeded": "service_unavailable",
        "GeneralError": "internal_server_error",
        "GenerateSecretKeyRequired": "forbidden",
        "InsufficientPrivilege": "forbidden",
        "InsufficientStorage": "insufficient_storage",
        "InternalError": "internal_server_error",
        "MaximumErrorsExceeded": "internal_server_error",
        "NoValidSession": "forbidden",
        "OperationFailed": "bad_gateway",
        "OperationNotAllowed": "method_not_allowed",
        "OperationTimeout": "internal_server_error",
        "PasswordChangeRequired": None,
        "PreconditionFailed": "precondition_failed",
        "PropertyNotWritable": "forbidden",
        "PropertyValueExternalConflict": "conflict",
        "PropertyValueModified": "ok",
        "PropertyValueResourceConflict": "conflict",
        "ResourceAtUriUnauthorized": "unauthorized",
        "ResourceCannotBeDeleted": "method_not_allowed",
        "ResourceExhaustion": "service_unavailable",
        "ResourceInStandby": "service_unavailable",
        "ResourceInUse": "service_unavailable",
        "ResourceNotFound": "not_found",
        "RestrictedRole": "forbidden",
        "ServiceDisabled": "service_unavailable",
        "ServiceInUnknownState": "service_unavailable",
        "ServiceShuttingDown": "service_unavailable",
        "ServiceTemporarilyUnavailable": "service_unavailable",
        "SessionLimitExceeded": "service_unavailable",
        "SessionTerminated": "ok",
        "SubscriptionTerminated": "ok",
        "Success": None,
    }

    return codes.get(entry_id, "bad_request")


def make_error_function(
    entry_id: str,
    entry: t.Dict[str, t.Any],
    is_header: bool,
    registry_name: str,
    namespace_name: str,
) -> str:
    struct_name = to_pascal_case(namespace_name)
    arg_nonstring_types = {
        "const boost::urls::url_view_base&": {
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
        },
        "const nlohmann::json&": {
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
        },
        "uint64_t": {
            "ArraySizeTooLong": [2],
            "InvalidIndex": [1],
            "StringValueTooLong": [2],
            "TaskProgressChanged": [2],
        },
    }

    out = ""
    args = []
    argtypes = []
    for arg_index, arg in enumerate(entry.get("ParamTypes", [])):
        arg_index += 1
        typename = "std::string_view"
        for typestring, entries in arg_nonstring_types.items():
            if arg_index in entries.get(entry_id, []):
                typename = typestring

        argtypes.append(typename)
        args.append(f"{typename} arg{arg_index}")
    function_name = entry_id[0].lower() + entry_id[1:]
    arg = ", ".join(args)
    out += f"nlohmann::json::object_t {function_name}({arg})"

    if is_header:
        out += ";\n\n"
    else:
        out += "\n{\n"
        to_array_type = ""
        arg_param = "{}"
        if argtypes:
            outargs = []
            for index, typename in enumerate(argtypes):
                index += 1
                if typename == "const nlohmann::json&":
                    out += f"std::string arg{index}Str = arg{index}.dump(-1, ' ', true, nlohmann::json::error_handler_t::replace);\n"
                elif typename == "uint64_t":
                    out += f"std::string arg{index}Str = std::to_string(arg{index});\n"

            for index, typename in enumerate(argtypes):
                index += 1
                if typename == "const boost::urls::url_view_base&":
                    outargs.append(f"arg{index}.buffer()")
                    to_array_type = "<std::string_view>"
                elif typename == "const nlohmann::json&":
                    outargs.append(f"arg{index}Str")
                    to_array_type = "<std::string_view>"
                elif typename == "uint64_t":
                    outargs.append(f"arg{index}Str")
                    to_array_type = "<std::string_view>"
                else:
                    outargs.append(f"arg{index}")
            argstring = ", ".join(outargs)
            arg_param = f"std::to_array{to_array_type}({{{argstring}}})"
        out += f"    return getLog(redfish::registries::{struct_name}::Index::{function_name}, {arg_param});"
        out += "\n}\n\n"
    if registry_name == "Base":
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
                out += "res.addHeader(boost::beast::http::field::retry_after, arg1);"

            res = get_response_code(entry_id)
            if res:
                out += f"    res.result(boost::beast::http::status::{res});\n"
            args_out = ", ".join([f"arg{x + 1}" for x in range(len(argtypes))])

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


def create_error_registry(
    registry_info: RegistryInfo,
    registry_name: str,
    namespace_name: str,
    filename: str,
) -> None:
    file, json_dict, namespace, url = registry_info
    base_filename = filename + "_messages"
    struct_name = to_pascal_case(namespace_name)

    error_messages_hpp = os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "include", f"{base_filename}.hpp"
    )
    messages = json_dict["Messages"]

    with open(
        error_messages_hpp,
        "w",
    ) as out:
        out.write(PRAGMA_ONCE)
        out.write(WARNING)
        out.write(
            """
// These generated headers are a superset of what is needed.
// clang sees them as an error, so ignore
// NOLINTBEGIN(misc-include-cleaner)
#include "http_response.hpp"

#include <boost/url/url_view_base.hpp>
#include <nlohmann/json.hpp>

#include <cstdint>
#include <source_location>
#include <string_view>
// NOLINTEND(misc-include-cleaner)

namespace redfish
{

namespace messages
{
"""
        )
        for entry_id, entry in messages.items():
            message = entry["Message"]
            for index in range(1, 10):
                message = message.replace(f"'%{index}'", f"<arg{index}>")
                message = message.replace(f"%{index}", f"<arg{index}>")

            if registry_name == "Base":
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
                out.write(
                    f"* @returns Message {entry_id} formatted to JSON */\n"
                )

            out.write(
                make_error_function(
                    entry_id, entry, True, registry_name, namespace_name
                )
            )
        out.write("    }\n")
        out.write("}\n")

    error_messages_cpp = os.path.join(
        SCRIPT_DIR, "..", "redfish-core", "src", f"{base_filename}.cpp"
    )
    with open(
        error_messages_cpp,
        "w",
    ) as out:
        out.write(WARNING)
        out.write(f'\n#include "{base_filename}.hpp"\n')
        headers = []

        headers.append('"registries.hpp"')
        if registry_name == "Base":
            reg_name_lower = "base"
            headers.append('"error_message_utils.hpp"')
            headers.append('"http_response.hpp"')
            headers.append('"logging.hpp"')
            headers.append("<boost/beast/http/field.hpp>")
            headers.append("<boost/beast/http/status.hpp>")
            headers.append("<boost/url/url_view_base.hpp>")
            headers.append("<source_location>")
        else:
            reg_name_lower = namespace_name.lower()
        headers.append(f'"registries/{reg_name_lower}_message_registry.hpp"')

        headers.append("<nlohmann/json.hpp>")
        headers.append("<array>")
        headers.append("<cstddef>")
        headers.append("<span>")

        if registry_name not in ("ResourceEvent", "HeartbeatEvent"):
            headers.append("<cstdint>")
            headers.append("<string>")
        headers.append("<string_view>")

        for header in headers:
            out.write(f"#include {header}\n")

        out.write(
            """
// Clang can't seem to decide whether this header needs to be included or not,
// and is inconsistent.  Include it for now
// NOLINTNEXTLINE(misc-include-cleaner)
#include <utility>

namespace redfish
{

namespace messages
{
"""
        )
        out.write(
            """
static nlohmann::json::object_t getLog(redfish::registries::{struct_name}::Index name,
                             std::span<const std::string_view> args)
{{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::{struct_name}::registry.size())
    {{
        return {{}};
    }}
    return getLogFromRegistry(redfish::registries::{struct_name}::header,
                              redfish::registries::{struct_name}::registry, index, args);
}}

""".format(
                struct_name=struct_name
            )
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
            out.write(
                make_error_function(
                    entry_id, entry, False, registry_name, namespace_name
                )
            )

        out.write("    }\n")
        out.write("}\n")
    os.system(f"clang-format -i {error_messages_hpp} {error_messages_cpp}")


def make_privilege_registry() -> None:
    path, json_file, type_name, url = make_getter(
        "Redfish_1.5.0_PrivilegeRegistry.json",
        "privilege_registry.hpp",
        "privilege",
    )
    with open(path, "w") as registry:
        registry.write(PRIVILEGE_HEADER)

        privilege_dict: t.Dict[str, t.Tuple[t.Any, str | None]] = {}
        for mapping in json_file["Mappings"]:
            # first pass, identify all the unique privilege sets
            for operation, privilege_list in mapping["OperationMap"].items():
                privilege_dict[
                    get_privilege_string_from_list(privilege_list)
                ] = (
                    privilege_list,
                    None,
                )
        for index, key in enumerate(privilege_dict):
            (privilege_list, _) = privilege_dict[key]
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


def to_pascal_case(text: str) -> str:
    s = text.replace("_", " ")
    s1 = s.split()
    if len(text) == 0:
        return text
    return "".join(i.capitalize() for i in s1[0:])


def main() -> None:
    dmtf_registries = OrderedDict(
        [
            ("base", "1.19.0"),
            ("composition", "1.1.2"),
            ("environmental", "1.1.0"),
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
        ]
    )

    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--registries",
        type=str,
        default="privilege,openbmc,"
        + ",".join([dmtf for dmtf in dmtf_registries]),
        help="Comma delimited list of registries to update",
    )

    args = parser.parse_args()

    registries = set(args.registries.split(","))
    registries_map: t.OrderedDict[str, RegistryInfo] = OrderedDict()

    for registry, version in dmtf_registries.items():
        if registry in registries:
            registry_pascal_case = to_pascal_case(registry)
            registries_map[registry] = make_getter(
                f"{registry_pascal_case}.{version}.json",
                f"{registry}_message_registry.hpp",
                registry,
            )
    if "openbmc" in registries:
        registries_map["openbmc"] = openbmc_local_getter()

    update_registries(list(registries_map.values()))

    if "base" in registries_map:
        create_error_registry(
            registries_map["base"],
            "Base",
            "base",
            "error",
        )
    if "heartbeat_event" in registries_map:
        create_error_registry(
            registries_map["heartbeat_event"],
            "HeartbeatEvent",
            "heartbeat_event",
            "heartbeat",
        )
    if "resource_event" in registries_map:
        create_error_registry(
            registries_map["resource_event"],
            "ResourceEvent",
            "resource_event",
            "resource",
        )
    if "task_event" in registries_map:
        create_error_registry(
            registries_map["task_event"],
            "TaskEvent",
            "task_event",
            "task",
        )

    if "privilege" in registries:
        make_privilege_registry()


if __name__ == "__main__":
    main()
