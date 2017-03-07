#! /usr/bin/python3

import argparse
import os
import gzip
import hashlib
from subprocess import Popen, PIPE

THIS_DIR = os.path.dirname(os.path.realpath(__file__))

BEGIN_BUFFER = """


#include <webassets.hpp>

void crow::webassets::request_routes(crow::App<crow::TokenAuthorizationMiddleware>& app){
"""

MIDDLE_BUFFER = """
    CROW_ROUTE(app, "{}")([](const crow::request& req, crow::response& res) {{
        res.code = 200;
        // TODO, if you have a browser from the dark ages that doesn't support gzip,
        // unzip it before sending based on Accept-Encoding header
        res.add_header("Content-Encoding", "gzip");

        res.write({{{}}});
        res.end();
        
    }});
"""


END_BUFFER = """

"""

def main():
    """ Main Function """
    file_list = []

    parser = argparse.ArgumentParser()
    parser.add_argument('-i', '--input', nargs='+', type=str)
    parser.add_argument('-o', '--output', type=str)
    args = parser.parse_args()

    file_list = args.input

    with open(args.output, 'w') as output_handle:

        output_handle.write(BEGIN_BUFFER)

        for full_filepath in file_list:

            pathsplit = full_filepath.split(os.path.sep)
            relative_path = os.path.sep.join(pathsplit[pathsplit.index("static") + 1:])

            relative_path = "/static/" + relative_path

            # handle the default routes
            if relative_path == "/static/index.html":
                relative_path = "/"

            # make sure none of the files are hidden
            with open(full_filepath, 'rb') as input_file:
                file_content = input_file.read()

            print("Including {:<40} size {:>7}".format(relative_path, len(file_content)))

            sha = hashlib.sha256()
            sha.update(file_content)

            sha_text = "".join("{:02x}".format(x) for x in sha.digest())

            print(sha_text)
            file_content = gzip.compress(file_content)
            #file_content = file_content[:10]

            array_binary_text = ', '.join('0x{:02x}'.format(x) for x in file_content)

            output_handle.write(MIDDLE_BUFFER.format(relative_path, array_binary_text))

        output_handle.write("};\n")
        output_handle.write(END_BUFFER)

if __name__ == "__main__":
    main()
