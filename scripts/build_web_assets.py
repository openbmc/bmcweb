#! /usr/bin/python3

import argparse
import os
import gzip
import hashlib
from subprocess import Popen, PIPE
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

CPP_BEGIN_BUFFER = """
#include <webassets.hpp>

"""

ROUTE_DECLARATION = """

void crow::webassets::request_routes(BmcAppType& app){
"""

CPP_MIDDLE_CACHING_HANDLER = """
        res.add_header("Cache-Control", "public, max-age=31556926");
        res.add_header("ETag", "{sha1}");
        if (req.headers.count("If-None-Match") == 1) {{
            if (req.get_header_value("If-None-Match") == "{sha1}"){{
                res.code = 304;
                res.end();
                return;
            }}
        }}
"""


CPP_MIDDLE_BUFFER = """
    CROW_ROUTE(app, "{relative_path_sha1}")([](const crow::request& req, crow::response& res) {{
        {CPP_MIDDLE_CACHING_HANDLER}
        res.code = 200;
        // TODO, if you have a browser from the dark ages that doesn't support gzip,
        // unzip it before sending based on Accept-Encoding header
        res.add_header("Content-Encoding", "{content_encoding}");
        res.add_header("Content-Type", "{content_type}");

        res.write({relative_path_escaped});
        
        res.end();
    }});
"""


def twos_comp(val, bits):
    """compute the 2's compliment of int value val"""
    if (val & (1 << (bits - 1))) != 0: # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is

CPP_END_BUFFER = """
}
"""

CPP_END_BUFFER2 = """const static std::string {relative_path_escaped}{{{file_bytes}}};
"""

def get_relative_path(full_filepath):
    pathsplit = full_filepath.split(os.path.sep)
    relative_path = os.path.sep.join(pathsplit[pathsplit.index("static") + 1:])
    relative_path_escaped = relative_path
    for character in ['/', '.', '-']:
        relative_path_escaped = relative_path_escaped.replace(character, "_")

    relative_path = "/static/" + relative_path

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
        key = key.lstrip("/")
        replace_name = get_sha1_path_from_relative(key, value)
        key = re.escape(key)
        string_content = re.sub("((src|href)=[\"'])(" + key + ")([\"'])", "\\1" + replace_name + "\\4", string_content)
    return string_content.encode()


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
    if not args.debug:
        # TODO(ed) most html and woff cacheable
        excluded_types = [".html", ".woff"]
        # sha1 hash everthing
        for full_filepath in file_list:
            if os.path.splitext(full_filepath)[1] not in excluded_types:
                with open(full_filepath, 'rb') as input_file:
                    file_content = input_file.read()
                sha = hashlib.sha1()
                sha.update(file_content)

                sha_text = "".join("{:02x}".format(x) for x in sha.digest())
                relative_path, relative_path_escaped = get_relative_path(full_filepath)
                sha1_list[relative_path] = sha_text

    with open(args.output, 'w') as cpp_output:
        cpp_output.write(CPP_BEGIN_BUFFER)

        for full_filepath in file_list:
            # make sure none of the files are hidden
            with open(full_filepath, 'rb') as input_file:
                file_content = input_file.read()
            relative_path, relative_path_escaped = get_relative_path(full_filepath)

            print("Including {:<40} size {:>7}".format(relative_path, len(file_content)))

            if relative_path.endswith(".html") or relative_path == "/":
                print("Fixing {}".format(relative_path))
                file_content = filter_html(sha1_list, file_content)

            if not args.debug:
                file_content = gzip.compress(file_content)
                #file_content = file_content[:10]
                # compute the 2s complement.  If you don't, you get narrowing warnings from gcc/clang
            
            array_binary_text = ', '.join(str(twos_comp(x, 8)) for x in file_content)

            cpp_output.write(
                CPP_END_BUFFER2.format(
                    relative_path=relative_path,
                    file_bytes=array_binary_text,
                    relative_path_escaped=relative_path_escaped
                )
            )

        cpp_output.write(ROUTE_DECLARATION)


        for full_filepath in file_list:
            relative_path, relative_path_escaped = get_relative_path(full_filepath)
            sha1 = sha1_list.get(relative_path, '')

            content_type = CONTENT_TYPES.get(os.path.splitext(relative_path)[1], "")
            if content_type == "":
                print("unknown content type for {}".format(relative_path))

            # handle the default routes
            if relative_path == "/static/index.html":
                relative_path = "/"

            relative_path_sha1 = get_sha1_path_from_relative(relative_path, sha1)

            content_encoding = 'none' if args.debug else 'gzip'

            environment = {
                'relative_path':relative_path,
                'relative_path_escaped': relative_path_escaped,
                'relative_path_sha1': relative_path_sha1,
                'sha1': sha1,
                'sha1_short': sha1[:20],
                'content_type': content_type,
                'ENABLE_CACHING': str(ENABLE_CACHING).lower(),
                'content_encoding': ''
            }
            if ENABLE_CACHING and sha1 != "":
                environment["CPP_MIDDLE_CACHING_HANDLER"] = CPP_MIDDLE_CACHING_HANDLER.format(
                    **environment
                )
            else:
                environment["CPP_MIDDLE_CACHING_HANDLER"] = ""

            content = CPP_MIDDLE_BUFFER.format(
                **environment
            )
            cpp_output.write(content)

        cpp_output.write(CPP_END_BUFFER)



if __name__ == "__main__":
    main()
