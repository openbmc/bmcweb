# Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import json
import jinja2
import os
import requests
import subprocess
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

include_path = os.path.realpath(
    os.path.join(
        SCRIPT_DIR,
        "..",
        "redfish-core",
        "include"))

proxies = {
    'https': os.environ.get("https_proxy", None)
}


files = []


def make_getter(dmtf_name, header_name):
    # clean up
    url = 'https://redfish.dmtf.org/registries/{}'.format(dmtf_name)
    dmtf = requests.get(url, proxies=proxies)
    dmtf.raise_for_status()
    json_file = json.loads(dmtf.text)
    path = os.path.join(include_path, header_name)
    return (path, json_file)


def main():
    files.append(make_getter('Update.1.0.0.json',
                             'update_messages.hpp'))
    for file, json_dict in files:
        try:
            # Remove the old files
            os.remove(file)
        except BaseException:
            print("{} not found".format(file))
            pass
        with open(file, 'w') as registry:
            search_path = SCRIPT_DIR+'/templates/'
            env = jinja2.Environment(loader=jinja2.FileSystemLoader(
                search_path), extensions=['jinja2.ext.do'])
            registry.write(env.get_template(
                'messages.tmpl').render(main=json_dict))
        subprocess.check_call(["clang-format-10", "-i", file])


if __name__ == "__main__":
    main()
