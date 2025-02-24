#!/usr/bin/env python3
import argparse
import json
import os
import typing as t
from collections import OrderedDict
from dataclasses import dataclass

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

namespace redfish::registries::{}
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

OPENBMC_REGISTRIES_BASE_URL: t.Final[str] = (
    "https://raw.githubusercontent.com/openbmc/bmcweb/refs/heads/master/redfish-core/include/registries/"
)

DBUS_IFACES_BASE_PATH: t.Final[str] = "../phosphor-dbus-interfaces"

DBUS_NAMESPACE_PREFIX: t.Final[str] = "xyz.openbmc_project."

RegistryInfo: t.TypeAlias = t.Tuple[
    str, t.Dict[str, t.Any], str, t.Optional[str]
]


@dataclass
class DBusFileInfo:
    path: str
    version: str


@dataclass
class DBusToRedfish:
    dbus_event_name: str  # without DBUS_NAMESPACE_PREFIX prefix
    registry_prefix: str
    redfish_message_id: str
    args: t.List[t.Tuple[str, str]]


def make_getter(
    dmtf_name: str, header_name: str, type_name: str
) -> RegistryInfo:
    url = "https://redfish.dmtf.org/registries/{}".format(dmtf_name)
    dmtf = requests.get(url, proxies=PROXIES)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text, object_pairs_hook=OrderedDict)
    path = os.path.join(INCLUDE_PATH, header_name)
    return (path, json_file, type_name, url)


def openbmc_local_getter():
    url = os.path.join(OPENBMC_REGISTRIES_BASE_URL, "openbmc.json")
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

            registry.write(REGISTRY_HEADER.format(namespace))
            # Parse the Registry header info
            description = json_dict.get("Description", "")
            registry.write(
                "const Header header = {{\n"
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
                "constexpr const char* url =\n"
                "    {url};\n"
                "\n"
                "constexpr std::array registry =\n"
                "{{\n".format(
                    json_dict=json_dict,
                    url=f'"{url}"' if url else "nullptr",
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


def get_response_code(entry_id: str) -> str:
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

    return codes.get(entry_id, "bad_request")


def make_error_function(
    entry_id: str,
    entry: t.Dict[str, t.Any],
    is_header: bool,
    registry_name: str,
    namespace_name: str,
) -> str:
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
    out += f"nlohmann::json {function_name}({arg})"

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
        out += f"    return getLog(redfish::registries::{namespace_name}::Index::{function_name}, {arg_param});"
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


def create_error_registry(
    registry_info: RegistryInfo,
    registry_name: str,
    namespace_name: str,
    filename: str,
) -> None:
    file, json_dict, namespace, url = registry_info
    base_filename = filename + "_messages"

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
static nlohmann::json getLog(redfish::registries::{namespace_name}::Index name,
                             std::span<const std::string_view> args)
{{
    size_t index = static_cast<size_t>(name);
    if (index >= redfish::registries::{namespace_name}::registry.size())
    {{
        return {{}};
    }}
    return getLogFromRegistry(redfish::registries::{namespace_name}::header,
                              redfish::registries::{namespace_name}::registry, index, args);
}}

""".format(
                namespace_name=namespace_name
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


def to_pascal_case(text: str) -> str:
    s = text.replace("_", " ")
    s = s.split()
    if len(text) == 0:
        return text
    return "".join(i.capitalize() for i in s[0:])


def to_snake_case(text: str) -> str:
    ret = ""
    for i, c in enumerate(text):
        if c.isupper():
            if i == 0:
                ret += c.lower()
            else:
                ret += "_" + c.lower()
        else:
            ret += c
    return ret


def openbmc_name_to_snake_case(text: str) -> str:
    if "openbmc_" not in text.lower():
        raise ValueError(f"Invalid OpenBMC name {text}")
    return to_snake_case(text.replace("OpenBMC_", "openbmc_")).replace(
        "__", "_"
    )


def update_dbus_mapping(
    dbus_registries_map: t.Dict[str, DBusToRedfish],
) -> None:
    file_name = os.path.realpath(
        os.path.join(
            SCRIPT_DIR,
            "..",
            "redfish-core",
            "include",
            "dbus_registries_map.hpp",
        )
    )

    try:
        os.remove(file_name)
    except BaseException:
        print("{} not found".format(file_name))

    with open(file_name, "w") as dbus_registries_map_fh:
        dbus_registries_map_fh.write(COPYRIGHT)
        dbus_registries_map_fh.write(PRAGMA_ONCE)
        dbus_registries_map_fh.write(WARNING)
        dbus_registries_map_fh.write(
            """
#include <string_view>
#include <unordered_map>
#include <vector>

namespace redfish::dbus_registries_map
{{

constexpr const std::string_view dbusNamespacePrefix{{"{dbus_namespace_prefix}"}};

struct EntryInfo
{{
    const char* RegistryName;
    const char* RedfishMessageId;
    const std::vector<std::pair<const std::string_view, const std::string_view>>
        ArgsInfo;
}};

// The key is the dbus event name without DBUS_NAMESPACE_PREFIX prefix
const std::unordered_map<std::string_view, const EntryInfo> dbusToRedfishMessageId = {{
""".format(
                dbus_namespace_prefix=DBUS_NAMESPACE_PREFIX,
            )
        )

        for dbus_name, entry in OrderedDict(
            sorted(dbus_registries_map.items())
        ).items():
            args = entry.args
            dbus_registries_map_fh.write(
                "    {\n"
                f'        "{dbus_name}",\n'
                "        EntryInfo{\n"
                f'            "{entry.registry_prefix}",\n'
                f'            "{entry.redfish_message_id}",\n'
            )
            if not args:
                dbus_registries_map_fh.write("            {},\n")
                dbus_registries_map_fh.write("        },\n")
            else:
                args_cpp_lines = [
                    f'                {{"{arg[0]}", "{arg[1]}"}},\n'
                    for arg in args
                ]
                dbus_registries_map_fh.write("            {\n")
                for line in args_cpp_lines:
                    dbus_registries_map_fh.write(line)
                dbus_registries_map_fh.write("            }},\n")
            dbus_registries_map_fh.write("    },\n")

        dbus_registries_map_fh.write(
            "};\n} // namespace redfish::dbus_registries_map\n"
        )
    os.system(f"clang-format -i {file_name}")


def parse_oem_openbmc_mapping(
    dbus_name: str,
    retistry_prefix: str,
    message_id: str,
    args: t.List[t.Dict[str, str]],
) -> DBusToRedfish:
    return DBusToRedfish(
        dbus_name,
        retistry_prefix,
        message_id,
        args=[(arg["Name"], arg["Type"]) for arg in args],
    )


def parse_dbus_oem_file_messages(
    file: DBusFileInfo,
    registries: t.Dict[str, RegistryInfo],
    dbus_registries_map: t.Dict[str, DBusToRedfish],
) -> None:
    full_path = os.path.join(DBUS_IFACES_BASE_PATH, file.path)
    with open(full_path, "rb") as fd:
        json_file = json.load(fd)
        messages = json_file.get("Messages", {})

        if not messages:
            return

        if file.version != json_file["RegistryVersion"]:
            raise ValueError(
                f"Version mismatch for {file.path}. "
                f"Expected {file.version}, got {json_file['RegistryVersion']}"
            )

        registry_prefix = json_file["RegistryPrefix"]
        registry_name = openbmc_name_to_snake_case(registry_prefix)
        path = os.path.join(
            INCLUDE_PATH, f"{registry_name}_message_registry.hpp"
        )
        if registry_name not in registries:
            registries[registry_name] = (
                path,
                json_file,
                registry_name,
                None,
            )
        else:
            for message_id, message in messages.items():
                if message_id in registries[registry_name][1]["Messages"]:
                    raise ValueError(
                        f"Message {message_id} already exists in registry "
                        f"{registry_name}. Required by {file}"
                    )
                registries[registry_name][1]["Messages"][message_id] = message

        # Parse oem openbmc mapping
        for message_id, message in messages.items():
            oem_openbmc_mapping = message.get("Oem", {}).get(
                "OpenBMC_Mapping", {}
            )
            if not oem_openbmc_mapping:
                continue

            dbus_event_name = oem_openbmc_mapping.get("Event")

            if not dbus_event_name.startswith(DBUS_NAMESPACE_PREFIX):
                raise ValueError(
                    f"Message {message_id} in {file} does not start "
                    f"with {DBUS_NAMESPACE_PREFIX} "
                )

            obfuscated_dbus_event_name = dbus_event_name[
                len(DBUS_NAMESPACE_PREFIX) :
            ]
            if obfuscated_dbus_event_name in dbus_registries_map:
                info = dbus_registries_map[obfuscated_dbus_event_name]
                raise ValueError(
                    f"Message {dbus_event_name} already exists in "
                    f"dbus_registries_map for {info}. Required by {file}"
                )

            dbus_registries_map[obfuscated_dbus_event_name] = (
                parse_oem_openbmc_mapping(
                    obfuscated_dbus_event_name,
                    registry_prefix,
                    message_id,
                    oem_openbmc_mapping.get("Args", []),
                )
            )


def parse_dbus_oem_file_oem(
    file: DBusFileInfo,
    registries: t.Dict[str, RegistryInfo],
    dbus_registries_map: t.Dict[str, DBusToRedfish],
) -> None:
    full_path = os.path.join(DBUS_IFACES_BASE_PATH, file.path)
    with open(full_path, "rb") as fd:
        json_file = json.load(fd)
        # If Oem dict contains mapping of DBus messages to existing messages
        # in other registries
        oem_openbmc_mapping = json_file.get("Oem", {}).get(
            "OpenBMC_Mapping", {}
        )
        if not oem_openbmc_mapping:
            return

        if file.version != json_file["RegistryVersion"]:
            raise ValueError(
                f"Version mismatch for {file.path}. "
                f"Expected {file.version}, got {json_file['RegistryVersion']}"
            )

        for dbus_event_name, dbus_event_info in oem_openbmc_mapping.items():
            obfuscated_dbus_event_name = dbus_event_name[
                len(DBUS_NAMESPACE_PREFIX) :
            ]
            if obfuscated_dbus_event_name in dbus_registries_map:
                info = dbus_registries_map[obfuscated_dbus_event_name]
                raise ValueError(
                    f"Message {dbus_event_name} already exists in "
                    f"dbus_registries_map for {info}. Required by {file}"
                )

            registry_prefix, message_id = dbus_event_info[
                "RedfishEvent"
            ].split(".")
            dbus_registries_map[obfuscated_dbus_event_name] = (
                parse_oem_openbmc_mapping(
                    obfuscated_dbus_event_name,
                    registry_prefix,
                    message_id,
                    dbus_event_info.get("Args", []),
                )
            )


def parse_dbus_oem_files(
    registries: t.Dict[str, RegistryInfo],
    dbus_registries_map: t.Dict[str, DBusToRedfish],
) -> None:
    files = [
        DBusFileInfo(
            "builddir/gen/xyz/openbmc_project/Sensor/Threshold.json", "1.0.1"
        ),
        DBusFileInfo(
            "builddir/gen/xyz/openbmc_project/State/Leak/Detector.json",
            "2.0.0",
        ),
        DBusFileInfo(
            "builddir/gen/xyz/openbmc_project/State/Leak/DetectorGroup.json",
            "1.0.0",
        ),
        DBusFileInfo(
            "builddir/gen/xyz/openbmc_project/State/Cable.json", "1.0.0"
        ),
        DBusFileInfo("builddir/gen/xyz/openbmc_project/Logging.json", "1.0.1"),
    ]

    # Do 2 passes, first pass to parse the messages, second pass to parse the oem
    # Reason is that we need to know the messages before we can parse the oem
    for file in files:
        parse_dbus_oem_file_messages(file, registries, dbus_registries_map)

    for file in files:
        parse_dbus_oem_file_oem(file, registries, dbus_registries_map)


def main():
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
    registries_map = OrderedDict()
    dbus_registries_map: t.Dict[str, DBusToRedfish] = {}

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

    parse_dbus_oem_files(registries_map, dbus_registries_map)
    update_dbus_mapping(dbus_registries_map)
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
