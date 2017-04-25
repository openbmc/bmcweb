#! /usr/bin/python3

import argparse
import os
import gzip
import hashlib
from subprocess import Popen, PIPE
from collections import defaultdict
import re

THIS_DIR = os.path.dirname(os.path.realpath(__file__))

ENABLE_CACHING = True

# TODO(ed) this needs to be better
CONTENT_TYPES = {
    '.css': "text/css;charset=UTF-8",
    '.html': "text/html;charset=UTF-8",
    '.js': "text/html;charset=UTF-8",
    '.png': "image/png;charset=UTF-8",
    '.woff': "application/x-font-woff",
}

CPP_MIDDLE_BUFFER = """  CROW_ROUTE(app, "{relative_path_sha1}")
  ([](const crow::request& req, crow::response& res) {{
    {CACHE_FOREVER_HEADER}
    res.add_header("ETag", "{sha1}");
    if (req.headers.count("If-None-Match") == 1) {{
      if (req.get_header_value("If-None-Match") == "{sha1}") {{
        res.code = 304;
        res.end();
        return;
      }}
    }}

    res.code = 200;
    // TODO, if you have a browser from the dark ages that doesn't support gzip,
    // unzip it before sending based on Accept-Encoding header
    res.add_header("Content-Encoding", "{content_encoding}");
    res.add_header("Content-Type", "{content_type}");

    res.write(staticassets::{relative_path_escaped});

    res.end();
  }});
"""


def twos_comp(val, bits):
    """compute the 2's compliment of int value val"""
    if (val & (1 << (bits - 1))) != 0:  # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is

def get_relative_path(full_filepath):
    pathsplit = full_filepath.split(os.path.sep)
    relative_path = os.path.sep.join(pathsplit[pathsplit.index("static") + 1:])

    relative_path_escaped = relative_path
    for character in ['/', '.', '-']:
        relative_path_escaped = relative_path_escaped.replace(character, "_")

    relative_path = "static/" + relative_path

    return relative_path, relative_path_escaped


def get_sha1_path_from_relative(relative_path, sha1):
    if sha1 != "":
        path, extension = os.path.splitext(relative_path)
        return path + "-" + sha1[:10] + extension
    else:
        return relative_path


def filter_html(sha1_list, file_content):
    string_content = file_content.decode()
    for key, value in sha1_list.items():
        replace_name = get_sha1_path_from_relative(key, value)
        string_content_new = re.sub(
            "((src|href)=[\"'])(" + re.escape(key) + ")([\"'])", "\\1" + replace_name + "\\4", string_content)
        if string_content_new != string_content:
            print("    Replaced {}".format(key))
            print("        With {}".format(replace_name))
            string_content = string_content_new

    return string_content.encode()


def embed_angular_templates(sha1_list, dependency_ordered_file_list, content_dict, file_content):
    string_content = file_content.decode()
    index = string_content.find("<script")
    if index == -1:
        raise Exception("Couldn't find first script tag in html?")
    preload_string = ""
    for full_filepath in dependency_ordered_file_list:
        relative_path, _ = get_relative_path(full_filepath)
        if re.search("partial-.*\\.html", relative_path):
            sha1_path = get_sha1_path_from_relative(relative_path, sha1_list[relative_path])

            print("full_filepath" + full_filepath)
            preload_string += (
                "<script type=\"text/ng-template\" id=\"" + sha1_path + "\">\n" +
                open(full_filepath, 'r').read() +
                "</script>\n"
            )

    for key in content_dict:
        print(key)
    string_content = string_content[:index] + preload_string + string_content[index:]
    return string_content.encode()

def filter_js(sha1_list, file_content):
    string_content = file_content.decode()
    for key, value in sha1_list.items():
        replace_name = get_sha1_path_from_relative(key, value)
        string_content_new = re.sub(key, replace_name, string_content)
        if string_content_new != string_content:
            print("    Replaced {}".format(key))
            print("    With {}".format(replace_name))
            string_content = string_content_new
    return string_content.encode()


def compute_sha1_and_update_dict(sha1_list, file_content, relative_path):
    sha = hashlib.sha1()
    sha.update(file_content)
    sha_text = sha.hexdigest()
    sha1_list[relative_path] = sha_text


def get_dependencies(dependency_list, full_filepath):
    r = []
    my_dependencies = dependency_list[full_filepath]
    r.extend(my_dependencies)
    sub_deps = []
    for dependency in my_dependencies:
        sub_deps += get_dependencies(dependency_list, dependency)
    r.extend(sub_deps)
    return r


def remove_duplicates_preserve_order(seq):
    seen = set()
    seen_add = seen.add
    return [x for x in seq if not (x in seen or seen_add(x))]


def main():
    """ Main Function """

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', nargs='+', type=str)
    parser.add_argument('-o', '--output', type=str)
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    file_list = args.input

    file_list = [os.path.realpath(f) for f in file_list]

    sha1_list = {}
    content_dict = {}

    depends_on = {}

    gzip_content = not(args.debug)

    for full_filepath in file_list:
        relative_path, relative_path_escaped = get_relative_path(full_filepath)
        text_file_types = ['.css', '.js', '.html']
        ext = os.path.splitext(relative_path)[1]
        depends_on[full_filepath] = []
        if ext in text_file_types:
            with open(full_filepath, 'r') as input_file:
                file_content = input_file.read()
            for full_replacename in file_list:
                relative_replacename, _ = get_relative_path(full_replacename)
                if ext == ".html":
                    match = re.search(
                        "((src|href)=[\"'])(" + relative_replacename + ")([\"'])", file_content)
                    if match:
                        depends_on[full_filepath].append(full_replacename)

                elif ext == ".js" or ext == ".css":
                    match = re.search(
                        "(\.\./)*" + relative_replacename, file_content)
                    if match:
                        depends_on[full_filepath].append(full_replacename)

    dependency_ordered_file_list = []
    for full_filepath in file_list:
        relative_path, relative_path_escaped = get_relative_path(full_filepath)
        deps = get_dependencies(depends_on, full_filepath)
        dependency_ordered_file_list.extend(deps)
        dependency_ordered_file_list.append(full_filepath)

    dependency_ordered_file_list = remove_duplicates_preserve_order(
        dependency_ordered_file_list)

    total_payload_size = 0
    for full_filepath in dependency_ordered_file_list:
        # make sure none of the files are hidden
        with open(full_filepath, 'rb') as input_file:
            file_content = input_file.read()
        relative_path, relative_path_escaped = get_relative_path(
            full_filepath)
        extension = os.path.splitext(relative_path)[1]

        print("Including {:<40} size {:>7}".format(
            relative_path, len(file_content)))

        if extension == ".html" or relative_path == "/":
            new_file_content = filter_html(sha1_list, file_content)
            if relative_path.endswith("index.html"):
                new_file_content = embed_angular_templates(sha1_list, dependency_ordered_file_list, content_dict, new_file_content)
        elif extension == ".js" or extension == ".css":
            new_file_content = filter_js(sha1_list, file_content)
        else:
            new_file_content = file_content

        file_content = new_file_content

        if gzip_content:
            file_content = gzip.compress(file_content)

        compute_sha1_and_update_dict(
            sha1_list, file_content, relative_path)
        content_dict[full_filepath] = file_content

        total_payload_size += len(file_content)

    with open(args.output.replace("cpp", "hpp"), 'w') as hpp_output:
        hpp_output.write("#pragma once\n"
                         "\n"
                         "#include <string>\n"
                         "\n"
                         "#include <crow/app.h>\n"
                         "#include <crow/http_request.h>\n"
                         "#include <crow/http_response.h>\n"
                         "\n"
                         "#include <crow/routing.h>\n"
                         "\n"
                         "namespace crow {\n"
                         "namespace webassets {\n"
                        )

        hpp_output.write("struct staticassets {\n")
        for full_filepath in dependency_ordered_file_list:
            relative_path, relative_path_escaped = get_relative_path(
                full_filepath)
            hpp_output.write(
                "  static const std::string {};\n".format(relative_path_escaped))
        hpp_output.write("};\n\n")
        hpp_output.write("template <typename... Middlewares>\n")
        hpp_output.write("void request_routes(Crow<Middlewares...>& app) {\n")

        for full_filepath in dependency_ordered_file_list:
            relative_path, relative_path_escaped = get_relative_path(
                full_filepath)
            sha1 = sha1_list.get(relative_path, '')

            content_type = CONTENT_TYPES.get(
                os.path.splitext(relative_path)[1], "")
            if content_type == "":
                print("unknown content type for {}".format(relative_path))

            # handle the default routes
            if relative_path == "static/index.html":
                relative_path = "/"
                relative_path_sha1 = "/"
            else:
                relative_path_sha1 = "/" + \
                    get_sha1_path_from_relative(relative_path, sha1)
            #print("relative_path_sha1: " + relative_path_sha1)
            #print("sha1: " + sha1)
            content_encoding = 'gzip' if gzip_content else 'none'

            environment = {
                'relative_path': relative_path,
                'relative_path_escaped': relative_path_escaped,
                'relative_path_sha1': relative_path_sha1,
                'sha1': sha1,
                'sha1_short': sha1[:20],
                'content_type': content_type,
                'content_encoding': content_encoding,
                "CACHE_FOREVER_HEADER": ""
            }

            if ENABLE_CACHING:
                # if we have a valid sha1, and we have a unique path to the resource
                # it can be safely cached forever
                if sha1 != "" and relative_path != relative_path_sha1:
                    environment["CACHE_FOREVER_HEADER"] = "res.add_header(\"Cache-Control\", \"public, max-age=31556926\");\n"

            content = CPP_MIDDLE_BUFFER.format(**environment)
            hpp_output.write(content)

        hpp_output.write("}\n}\n}")

    with open(args.output, 'w') as cpp_output:
        cpp_output.write("#include <webassets.hpp>\n"
                         "namespace crow{\n"
                         "namespace webassets{\n")

        for full_filepath in dependency_ordered_file_list:
            file_content = content_dict[full_filepath]
            relative_path, relative_path_escaped = get_relative_path(
                full_filepath)
            # compute the 2s complement for negative numbers.
            # If you don't, you get narrowing warnings from gcc/clang
            array_binary_text = ', '.join(str(twos_comp(x, 8))
                                          for x in file_content)
            cpp_end_buffer = "const std::string staticassets::{relative_path_escaped}{{{file_bytes}}};\n"
            cpp_output.write(
                cpp_end_buffer.format(
                    relative_path=relative_path,
                    file_bytes=array_binary_text,
                    relative_path_escaped=relative_path_escaped
                )
            )
        cpp_output.write("}\n}\n")

    print("Total static file size: {}KB".format(int(total_payload_size/1024)))

if __name__ == "__main__":
    main()
