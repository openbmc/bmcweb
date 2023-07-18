#!/usr/bin/python3
import os
import re
from subprocess import check_output

"""
This script is intended to aid in the replacement of bmcweb log lines from
BMCWEB_LOG_CRITICAL << "foo" << arg;
to the new fmt style form
BMCWEB_LOG_CRITICAL("foo{}", arg);

It is intended to be a 99% correct tool, and in some cases, some amount of
manual intervention is required once the conversion has been done.  Some
things it doesn't handle:

ptrs need to be wrapped in logPtr().  This script tries to do the common
cases, but can't handle every one.

types of boost::ip::tcp::endpoint/address needs .to_string() called on it
in the args section

arguments that were previously build by in-line operator+ construction of
strings (for example "foo" + bar), need to be formatted in the arguments
brace replacement language, (for example "foo{}", bar)

After the script has been run, remember to reformat with clang-format

This script will be removed Q2 2024
"""


SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))

files = check_output(
    ["git", "-C", os.path.join(SCRIPT_DIR, ".."), "grep", "-l", "BMCWEB_LOG_"]
)
filenames = files.split(b"\n")


for filename in filenames:
    if not filename:
        continue
    with open(filename, "r") as filehandle:
        contents = filehandle.read()

    # Normalize all the log statements to a single line
    new_contents = ""
    replacing = False
    for line in contents.splitlines():
        match = re.match(r"\s+BMCWEB_LOG_", line)
        if match:
            replacing = True

        if replacing and not match:
            line = " " + line.lstrip()
        if line.endswith(";"):
            replacing = False
        new_contents += line
        if not replacing:
            new_contents += "\n"
    contents = new_contents

    new_contents = ""
    for line in contents.splitlines():
        match = re.match(r"(\s+BMCWEB_LOG_[A-Z]+) <<", line)
        if match:
            logstring = ""
            line = line.lstrip()
            chevron_split = line.split("<<")
            arguments = []
            for log_piece in chevron_split[1:]:
                if log_piece.endswith(";"):
                    log_piece = log_piece[:-1]
                log_piece = log_piece.strip()
                if (log_piece.startswith('"') and log_piece.endswith('"')) or (
                    log_piece.startswith("'") and log_piece.endswith("'")
                ):
                    logstring += log_piece[1:-1]
                else:
                    if log_piece == "this" or log_piece.startswith("&"):
                        log_piece = "logPtr({})".format(log_piece)
                    if log_piece == "self":
                        log_piece = "logPtr(self.get())"
                    arguments.append(log_piece)
                    logstring += "{}"
            if logstring.endswith("\\n"):
                logstring = logstring[:-2]

            arguments.insert(0, '"' + logstring + '"')
            argjoin = ", ".join(arguments)

            line = match.group(1) + "(" + argjoin + ");"
        new_contents += line + "\n"

    contents = new_contents

    with open(filename, "w") as filehandle:
        filehandle.write(new_contents)

print()
