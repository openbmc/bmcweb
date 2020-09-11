#!/usr/bin/python3

import argparse
import json
import re


parser = argparse.ArgumentParser()
parser.add_argument('-b', '--base', default=None, required=True)
parser.add_argument('-f', '--file', default=None, required=True)
args = parser.parse_args()

with open(args.file) as error_file:
    error_data = error_file.readlines()

    with open(args.base) as base_file:
        base_json = json.load(base_file)
        for message_name, message_data in base_json['Messages'].items():
            message_id = base_json['RegistryPrefix'] + '.' + \
                base_json['RegistryVersion'] + '.' + message_name
            message_found = False
            index = 0
            for i, error in enumerate(error_data):
                if message_id in error:
                    message_found = True
                    index = i
                    break
            if not message_found:
                print("{} not found".format(message_id))
                continue

            error_info = error_data[index:index + 15]
            error_str = " ".join(error_info)
            error_str = re.sub(
                'std::to_string\(arg(\d+)\)',
                'arg\\1',
                error_str)
            error_str = re.sub('"\n\s*"', '', error_str, re.MULTILINE)
            error_str = re.sub(
                '"\s*\+\s*arg(\d+)\s*\+\n?\s*"',
                '%\\1',
                error_str,
                re.MULTILINE)
            if message_data['Message'] not in error_str:
                print(
                    "{}: error in Message: {}".format(
                        message_id,
                        message_data['Message']))
                print(error_str)

            if message_data['MessageSeverity'] not in error_str:
                print("{}: error in MessageSeverity: {}".format(
                    message_id, message_data['MessageSeverity']))
                print(error_str)

            if message_data['Resolution'] not in error_str:
                print(
                    "{}: error in Resolution: {}".format(
                        message_id,
                        message_data['Resolution']))
                print(error_str)
