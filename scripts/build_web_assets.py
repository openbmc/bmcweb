#! /usr/bin/python3

import argparse
import os
import gzip
import hashlib
from subprocess import Popen, PIPE
from collections import defaultdict
import re

THIS_DIR = os.path.dirname(os.path.realpath(__file__))

ENABLE_CACHING = False

# TODO(ed) THis should really pull type and file information from webpack
CONTENT_TYPES = {
    'css': "text/css;charset=UTF-8",
    'html': "text/html;charset=UTF-8",
    'js': "text/html;charset=UTF-8",
    'png': "image/png;charset=UTF-8",
    'woff': "application/x-font-woff",
    'woff2': "application/x-font-woff2",
    'ttf': "application/x-font-ttf",
    "svg": "image/svg+xml",
    "eot": "application/vnd.ms-fontobject",
    # dev tools don't care, causes browser to show as text
    # https://stackoverflow.com/questions/19911929/what-mime-type-should-i-use-for-javascript-source-map-files
    "map": "application/json"
}

CPP_MIDDLE_BUFFER = """  CROW_ROUTE(app, "{pretty_name}")
  ([](const crow::request& req, crow::response& res) {{
    {CACHE_FOREVER_HEADER}
    std::string sha1("{sha1}");
    res.add_header(etag_string, sha1);

    if (req.get_header_value(if_none_match_string) == sha1) {{
      res.code = 304;
    }} else {{
        res.code = 200;
        // TODO, if you have a browser from the dark ages that doesn't support gzip,
        // unzip it before sending based on Accept-Encoding header
        res.add_header(content_encoding_string, {content_encoding});
        res.add_header(content_type_string, "{content_type}");

        res.write(staticassets::{relative_path_escaped});
    }}
    res.end();
  }});

"""

HPP_START_BUFFER = ("#pragma once\n"
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
                    "static const char* gzip_string = \"gzip\";\n"
                    "static const char* none_string = \"none\";\n"
                    "static const char* if_none_match_string = \"If-None-Match\";\n"
                    "static const char* content_encoding_string = \"Content-Encoding\";\n"
                    "static const char* content_type_string = \"Content-Type\";\n"
                    "static const char* etag_string = \"ETag\";\n"
                    )


def twos_comp(val, bits):
    """compute the 2's compliment of int value val"""
    if (val & (1 << (bits - 1))) != 0:  # if sign bit is set e.g., 8bit: 128-255
        val = val - (1 << bits)        # compute negative value
    return val                         # return positive value as is


def main():
    """ Main Function """

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', type=str)
    parser.add_argument('-o', '--output', type=str)
    parser.add_argument('-d', '--debug', action='store_true')
    args = parser.parse_args()

    dist_dir = args.input

    with open(args.output.replace("cpp", "hpp"), 'w') as hpp_output:
        hpp_output.write(HPP_START_BUFFER)
        hpp_output.write("struct staticassets {\n")

        asset_filenames = []

        for root, dirnames, filenames in os.walk(dist_dir):
            for filename in filenames:
                root_file = os.path.join(root, filename)
                pretty_name = "/" + os.path.relpath(root_file, dist_dir)
                cpp_name = "file" + pretty_name
                for character in ['/', '.', '-']:
                    cpp_name = cpp_name.replace(character, "_")

                if pretty_name.endswith(".gz"):
                    pretty_name = pretty_name[:-3]
                    gzip = True
                else:
                    gzip = False

                if pretty_name.endswith("/index.html"):
                    pretty_name = pretty_name[:-10]

                asset_filenames.append(
                    (root_file, pretty_name, cpp_name, gzip))

        for root_file, pretty_name, cpp_name, gzip in asset_filenames:

            with open(root_file, 'rb') as file_handle:
                file_content = file_handle.read()

            hpp_output.write(
                "  static const std::array<char, {}> {};\n".format(len(file_content), cpp_name))
        hpp_output.write(
            "  static const std::array<const char*, {}> routes;\n".format(len(asset_filenames)))
        hpp_output.write("};\n\n")
        hpp_output.write("template <typename... Middlewares>\n")
        hpp_output.write(
            "void request_routes(Crow<Middlewares...>& app) {\n")

        for root_file, pretty_name, cpp_name, gzip in asset_filenames:
            os.path.basename(root_file)
            with open(root_file, 'rb') as file_handle:
                file_content = file_handle.read()
                sha = hashlib.sha1()
                sha.update(file_content)
                sha1 = sha.hexdigest()

            ext = os.path.split(root_file)[-1].split(".")[-1]
            if ext == "gz":
                ext = os.path.split(root_file)[-1].split(".")[-2]

            content_type = CONTENT_TYPES.get(ext, "")
            if content_type == "":
                print("unknown content type for {}".format(pretty_name))

            content_encoding = 'gzip_string' if gzip else 'none_string'

            environment = {
                'relative_path': pretty_name,
                'relative_path_escaped': cpp_name,
                'pretty_name': pretty_name,
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

        hpp_output.write(
            "}  // namespace staticassets\n}  // namespace webassets\n}  // namespace crow")

        with open(args.output, 'w') as cpp_output:
            cpp_output.write("#include <webassets.hpp>\n"
                             "namespace crow{\n"
                             "namespace webassets{\n")

            for root_file, pretty_name, cpp_name, gzip in asset_filenames:
                with open(root_file, 'rb') as file_handle:
                    file_content = file_handle.read()
                # compute the 2s complement for negative numbers.
                # If you don't, you get narrowing warnings from gcc/clang
                array_binary_text = ', '.join(str(twos_comp(x, 8))
                                              for x in file_content)
                cpp_end_buffer = "  const std::array<char, {byte_length}> staticassets::{relative_path_escaped} = {{{file_bytes}}};\n"
                cpp_output.write(
                    cpp_end_buffer.format(
                        relative_path_escaped=cpp_name,
                        byte_length=len(file_content),
                        relative_path=pretty_name,
                        file_bytes=array_binary_text
                    )
                )
                print("{:<40} took {:>6} KB".format(
                    pretty_name, int(len(array_binary_text) / 1024)))
            static_routes = ",\n".join(
                ['    "' + x[1] + '"' for x in asset_filenames])
            cpp_output.write(
                "\n  const std::array<const char*, {}> staticassets::routes{{\n{}}};\n".format(len(asset_filenames), static_routes))
            cpp_output.write(
                "}  // namespace webassets\n}  // namespace crow\n")


if __name__ == "__main__":
    main()
