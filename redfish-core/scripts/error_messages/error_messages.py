# Copyright (c) 2018 Intel Corporation
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import json
import os
import re
import sys

# CONSTANTS
# Containing namespace
NAMESPACE = "redfish::messages::"
# Single indentation
INDENT = " " * 2
# Include directory where created files will be stored
AUTOGEN_PATH = ""
# MessageRegistry JSON Path
MESSAGE_REGISTRY_JSON_PATH = ""
# Error messages target filename
ERROR_MESSAGES_TARGET_FILENAME = "error_messages"
# Script path
SCRIPT_PATH = os.path.dirname(sys.argv[0])

FORMATTING_FUNCTIONS_DECLARATIONS = []
FORMATTING_FUNCTIONS_IMPLEMENTATIONS = []

# Loaded MessageRegistry JSON
MESSAGE_REGISTRY = {}

# Path to output header file
OUT_HEADER_PATH = ""
# Path to output source file
OUT_SOURCE_PATH = ""

PARAM_TYPE_TO_CPP_TYPE = {
    "string": "std::string",
    "number": "int"
}

PARAM_TYPE_TO_FORMATTING_ARG = {
    "string": 'arg{}',
    "number": 'std::to_string(arg{})'
}


def verify_message(message_id):
    """
    Verifies message, by checking that all its parameter information is correct

    :param message_id: Id to be verified
    :return: True if message is OK, False otherwise
    """
    try:
        message_body = MESSAGE_REGISTRY["Messages"][message_id]["Message"]
        message_num_args = MESSAGE_REGISTRY["Messages"][message_id]["NumberOfArgs"]
        regex_find_all_args = '(%[0-9]{1})'
        found_args = re.findall(regex_find_all_args, message_body)

        if message_num_args == 0:
            # Verify that there are no %<digit> in message body
            if len(found_args) != 0:
                return False
        else:
            param_types = MESSAGE_REGISTRY["Messages"][message_id]["ParamTypes"]

            if message_num_args != len(param_types):
                return False
            else:
                # Verify that param types are not unknown
                for param_type in param_types:
                    if not param_type in PARAM_TYPE_TO_CPP_TYPE:
                        print "Unknown param type " + param_type

                        return False

                # Verify that there are as many args as there should be
                if len(found_args) != message_num_args:
                    return False

                # Verify that all required args exist
                for i in range(1, message_num_args + 1):
                    found_args.remove("%" + str(i))

                if len(found_args) != 0:
                    return False
    except:
        return False

    return True


def create_function_brief(message_id):
    """
    Creates dOxygen brief for formatting function of given message

    :param message_id: Id of message for which brief should be created
    :return: Created @brief message
    """
    return "@brief Formats " + message_id + " message into JSON"


def create_message_body_for_doxy(message_id):
    """
    Creates formatted message body for dOxygen comment to lookup message without looking into function code

    :param message_id: Id of message for which message body for doxy should be created
    :return: Created message
    """
    body = MESSAGE_REGISTRY["Messages"][message_id]["Message"]
    num_params = MESSAGE_REGISTRY["Messages"][message_id]["NumberOfArgs"]

    for param_idx in range(0, num_params):
        formatting_str = '%' + str(param_idx + 1)

        body = body.replace(formatting_str, '<arg' + str(param_idx) + '>');

    return 'Message body: "' + body + '"'


def create_full_doxygen_comment(message_id):
    """
    Creates full dOxygen comment with given data

    :param message_id: Id of message for which's formatting function comment should be created
    :return:
    """
    num_params = MESSAGE_REGISTRY["Messages"][message_id]["NumberOfArgs"]

    out_comment = "/**\n" + \
        " * " + create_function_brief(message_id) + "\n" \
        " * " + create_message_body_for_doxy(message_id) + "\n" \
        " *\n"

    for param_idx in range(1, num_params + 1):
        out_comment += " * @param[in] arg" + str(param_idx) + " Parameter of message that will replace %" +\
                       str(param_idx) + " in its body.\n"

    out_comment += " *\n"
    out_comment += " * @returns Message " + message_id + " formatted to JSON"
    out_comment += " */\n"

    return out_comment


def create_internal_doxygen_comment(message_id):
    """
    Creates internal dOxygen comment with given data

    :param message_id: Id of message for which's formatting function comment should be created
    :return:
    """
    out_comment = "/**\n" + \
        " * @internal\n" + \
        " * " + create_function_brief(message_id) + "\n" + \
        " *\n" + \
        " * See header file for more information\n" + \
        " * @endinternal\n" + \
        " */\n"

    return out_comment


def create_message_function_head(message_id):
    """
    Create header like nlohmann::json <msg_id_with_first_letter_lowercase>(...)

    :param message_id: Id of message for which function header should be created
    :return: Created function header
    """
    function_name = message_id[:1].lower() + message_id[1:]
    function_arguments = ""

    if MESSAGE_REGISTRY["Messages"][message_id]["NumberOfArgs"] != 0:
        param_types = MESSAGE_REGISTRY["Messages"][message_id]["ParamTypes"]
        params = []

        for param_idx in range(0, len(param_types)):
            params.append("const " + PARAM_TYPE_TO_CPP_TYPE[param_types[param_idx]] + "& arg" + str(param_idx + 1))

        function_arguments = ', '.join(params)

    return "nlohmann::json " + function_name + "(" + function_arguments + ")"


def create_message_function_body(message_id, is_complex):
    """
    Creates format function body for message without parameters

    :param message_id: Id of message for which format function body should be created
    :return: Format function body for given message
    """
    message_data = MESSAGE_REGISTRY["Messages"][message_id]
    message_body = message_data["Message"]

    if is_complex:
        param_types = message_data["ParamTypes"]

        for param_idx in range(0, len(param_types)):
            formatting_str = '%' + str(param_idx + 1)
            formatting_arg = PARAM_TYPE_TO_FORMATTING_ARG[param_types[param_idx]].format(str(param_idx + 1))

            message_body = message_body.replace(formatting_str, '" + ' + formatting_arg + ' + "');

    body = ' {\n' + \
        INDENT + 'return nlohmann::json{' + '\n' + \
        INDENT + INDENT + '{"@odata.type", "/redfish/v1/$metadata#Message.v1_0_0.Message"},' + '\n' + \
        INDENT + INDENT + '{"MessageId", "' + MESSAGE_REGISTRY["Id"] + "." + message_id + '"},' + '\n' + \
        INDENT + INDENT + '{"Message", "' + message_body + '"},' + '\n' + \
        INDENT + INDENT + '{"Severity", "' + message_data["Severity"] + '"},' + '\n' + \
        INDENT + INDENT + '{"Resolution", "' + message_data["Resolution"] + '"}' + '\n' + \
        INDENT + '};' + '\n' + \
        '}'

    return body


def process_message(message_id):
    """
    Processes given message and adds created definitions to appropriate globals
     - Creates MessageId enumeration value for given message
     - Creates macro to use given message in format:
       a) For argument-less messages - FORMAT_REDFISH_MESSAGE_<message_id>
       b) For parametrized messages - FORMAT_REDFISH_MESSAGE_<message_id>(arg1, arg2, ...)
     - Creates structure definition for message

    :param message_id: ID of message that should be processed

    :return: None
    """
    if verify_message(message_id):
        foo_head = create_message_function_head(message_id)
        foo_body = create_message_function_body(message_id,
                                                MESSAGE_REGISTRY["Messages"][message_id]["NumberOfArgs"] != 0)

        # For header file
        full_doxy_comment = create_full_doxygen_comment(message_id)

        FORMATTING_FUNCTIONS_DECLARATIONS.append(full_doxy_comment + foo_head + ';\n')

        # For source file
        internal_doxy_comment = create_internal_doxygen_comment(message_id)

        format_foo_impl = internal_doxy_comment + foo_head + foo_body

        FORMATTING_FUNCTIONS_IMPLEMENTATIONS.append(format_foo_impl)
    else:
        print message_id + " is not correctly defined! It will be omitted"


def process_messages():
    """
    Iterates through all messages and processes each of them

    :return: None
    """
    for message_id in MESSAGE_REGISTRY[u'Messages']:
        process_message(message_id)


def prepare_declarations():
    return '\n'.join(FORMATTING_FUNCTIONS_DECLARATIONS)


def prepare_implementations():
    return '\n\n'.join(FORMATTING_FUNCTIONS_IMPLEMENTATIONS)


def generate_error_messages_cpp():
    """
    Replaces placeholders inside source file with generated data

    :return: None
    """
    with open(SCRIPT_PATH + "/" + ERROR_MESSAGES_TARGET_FILENAME + ".cpp.in", 'r') as source_file_template:
        file_contents = source_file_template.read()

        file_contents = file_contents.replace("/*<<<<<<AUTOGENERATED_FUNCTIONS_IMPLEMENTATIONS>>>>>>*/",
                                              prepare_implementations())

        with open(OUT_SOURCE_PATH, "w") as source_file_target:
            source_file_target.write(file_contents)


def generate_error_messages_hpp():
    """
    Replaces placeholders inside header file with generated data

    :return: None
    """
    with open(SCRIPT_PATH + "/" + ERROR_MESSAGES_TARGET_FILENAME + ".hpp.in", 'r') as header_file_template:
        file_contents = header_file_template.read()

        # Add definitions
        file_contents = file_contents.replace("/*<<<<<<AUTOGENERATED_FUNCTIONS_DECLARATIONS>>>>>>*/",
                                              prepare_declarations())

        with open(OUT_HEADER_PATH, "w") as header_file_target:
            header_file_target.write(file_contents)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        sys.exit("Not enough arguments. Required syntax is:\n error_messages.py <autogen_dir> <messages_json>")
    else:
        AUTOGEN_PATH = sys.argv[1]
        MESSAGE_REGISTRY_JSON_PATH = sys.argv[2]

    MESSAGE_REGISTRY = json.load(open(MESSAGE_REGISTRY_JSON_PATH))

    OUT_HEADER_PATH = AUTOGEN_PATH + "/" + ERROR_MESSAGES_TARGET_FILENAME + ".hpp"
    OUT_SOURCE_PATH = AUTOGEN_PATH + "/" + ERROR_MESSAGES_TARGET_FILENAME + ".cpp"

    # Make sure that files directory exists
    if not os.path.exists(AUTOGEN_PATH):
        os.makedirs(AUTOGEN_PATH)

    # Process found messages
    process_messages()

    # Generate files
    generate_error_messages_cpp()
    generate_error_messages_hpp()
