#! /usr/bin/python3

import argparse
import os
import gzip
import hashlib
from subprocess import Popen, PIPE

THIS_DIR = os.path.dirname(os.path.realpath(__file__))

ENABLE_CACHING = False

# TODO this needs to be better
CONTENT_TYPES = {
    '.css': "text/css;charset=UTF-8",
    '.html': "text/html;charset=UTF-8",
    '.js': "text/html;charset=UTF-8",
    '.png': "image/png;charset=UTF-8"
}

CPP_BEGIN_BUFFER = """
#include <webassets.hpp>

"""

ROUTE_DECLARATION = """void crow::webassets::request_routes(crow::App<crow::TokenAuthorizationMiddleware>& app){
"""

CPP_MIDDLE_BUFFER = """
    CROW_ROUTE(app, "{relative_path_sha1}")([](const crow::request& req, crow::response& res) {{
        res.code = 200;
        // TODO, if you have a browser from the dark ages that doesn't support gzip,
        // unzip it before sending based on Accept-Encoding header
        res.add_header("Content-Encoding", "gzip");
        res.add_header("Cache-Control", "{cache_control_value}");
        res.add_header("Content-Type", "{content_type}");

        res.write({relative_path_escaped});
        res.end();
        
    }});
"""


CPP_END_BUFFER = """
}
"""

CPP_END_BUFFER2 = """const static std::string {relative_path_escaped}{{{file_bytes}}};
"""

def get_relative_path(full_filepath):
    pathsplit = full_filepath.split(os.path.sep)
    relative_path = os.path.sep.join(pathsplit[pathsplit.index("static") + 1:])

    relative_path_escaped = relative_path.replace("/", "_").replace(".", "_").replace("-", "_")

    relative_path = "/static/" + relative_path

    # handle the default routes
    if relative_path == "/static/index.html":
        relative_path = "/"

    return relative_path, relative_path_escaped

def get_sha1_path_from_relative(relative_path, sha1):
    if sha1 != "":
        path, extension = os.path.splitext(relative_path)
        return path + "_" + sha1 + extension
    else:
        return relative_path


def filter_html(sha1_list, file_content):
    string_content = file_content.decode()
    for key, value in sha1_list.items():
        if key != "/":
            # todo, this is very naive, do it better (parse the html)
            start = "src=\"" + key.lstrip("/")
            replace = "src=\"" + get_sha1_path_from_relative(key, value)
            #print("REplacing {} with {}".format(start, replace))
            string_content = string_content.replace(start, replace)

            start = "href=\"" + key.lstrip("/")
            replace = "href=\"" + get_sha1_path_from_relative(key, value)
            #print("REplacing {} with {}".format(start, replace))
            string_content = string_content.replace(start, replace)
    return string_content.encode()

def main():
    """ Main Function """

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', nargs='+', type=str)
    parser.add_argument('-o', '--output', type=str)
    args = parser.parse_args()

    file_list = args.input

    sha1_list = {}
    if ENABLE_CACHING:
        # Sha256 hash everthing
        for full_filepath in file_list:
            if not full_filepath.endswith(".html"):
                with open(full_filepath, 'rb') as input_file:
                    file_content = input_file.read()
                sha = hashlib.sha256()
                sha.update(file_content)

                sha_text = "".join("{:02x}".format(x) for x in sha.digest())[:10]
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


            file_content = gzip.compress(file_content)
            #file_content = file_content[:10]

            array_binary_text = ', '.join('0x{:02x}'.format(x) for x in file_content)

            cpp_output.write(CPP_END_BUFFER2.format(relative_path=relative_path, file_bytes=array_binary_text, relative_path_escaped=relative_path_escaped))

        cpp_output.write(ROUTE_DECLARATION)


        for full_filepath in file_list:
            relative_path, relative_path_escaped = get_relative_path(full_filepath)
            sha1 = sha1_list.get(relative_path, '')

            relative_path_sha1 = get_sha1_path_from_relative(relative_path, sha1)

            content_type = CONTENT_TYPES.get(os.path.splitext(relative_path)[1], "")
            if content_type == "":
                print("unknown content type for {}".format(relative_path))

            if sha1 == "":
                cache_control_value = "no-cache"
            else:
                cache_control_value = "max-age=31556926"

            content = CPP_MIDDLE_BUFFER.format(
                relative_path=relative_path,
                relative_path_escaped=relative_path_escaped,
                relative_path_sha1=relative_path_sha1,
                sha1=sha1,
                content_type=content_type,
                cache_control_value=cache_control_value
            )
            cpp_output.write(content)

        cpp_output.write(CPP_END_BUFFER)



if __name__ == "__main__":
    main()
