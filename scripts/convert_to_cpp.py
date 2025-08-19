#!/usr/bin/env python3

"""
Script to convert hpp files in redfish-core/lib to separate hpp/cpp files.
"""

import argparse
import os
import re
import sys
from pathlib import Path
from typing import Dict, List, Tuple

header_regexes = [
    r"request\w*Routes\w*",
    r"getCertificateProperties",
    r"getNTPProtocolEnabled",
    r"handleHypervisorSystemResetPost",
    r"handleHypervisorResetActionGet",
    r"handleHypervisorSystemGet",
    r"requestAccountServiceRoutes",  # WHYYYYY? is this one special?
    r"getChassisData",
    r"getRoleIdFromPrivilege",
    r"getMetadataPieceForFile",
    r"handleChassisProperties",
    r"translateChassisTypeToRedfish",
    r"handleChassisResetActionInfoGet",
    r"getDumpServiceInfo",
    r"afterGetAllowedHostTransitions",
    r"doPowerSubsystemCollection",
    r"setBytesProperty",
    r"setPercentProperty",
    r"parsePostCode",
    r"handleServiceRootGetImpl",
    r"redfishOdataGet",
    r"doThermalSubsystemCollection",
    r"parseSimpleUpdateUrl",
]


class HeaderConverter:
    def __init__(self, project_root: Path):
        self.project_root = project_root
        self.redfish_core_lib = project_root / "redfish-core" / "lib"
        self.redfish_core_src = project_root / "redfish-core" / "src"
        self.meson_build = project_root / "meson.build"

        # Pattern to find inline function definitions
        self.inline_function_pattern = re.compile(
            r"^\s*inline\s+([^{]*?)\s*\{", re.MULTILINE | re.DOTALL
        )

    def extract_includes(self, content: str) -> Tuple[List[str], str]:
        """Extract include statements from the content."""
        lines = content.split("\n")
        includes = []
        include_end = 0

        for i, line in enumerate(lines):
            stripped = line.strip()
            if stripped.startswith("#include") or stripped.startswith(
                "#pragma"
            ):
                includes.append(line)
                include_end = i
            elif (
                stripped
                and not stripped.startswith("//")
                and not stripped.startswith("/*")
            ):
                break

        # Also capture copyright header and license
        header_lines = []
        for i, line in enumerate(lines):
            if i <= include_end:
                if line.strip().startswith("//") and (
                    "SPDX" in line or "Copyright" in line or "Apache" in line
                ):
                    header_lines.append(line)
                elif line.strip().startswith("#"):
                    break

        remaining_content = "\n".join(lines[include_end + 1 :])
        return includes, remaining_content

    def extract_namespace_content(self, content: str) -> Tuple[str, str, str]:
        """Extract content inside redfish namespace."""
        # Find the redfish namespace
        namespace_pattern = re.compile(
            r"^\s*namespace\s+redfish\s*\{(.*?)\}\s*//\s*namespace\s+redfish\s*$",
            re.MULTILINE | re.DOTALL,
        )

        match = namespace_pattern.search(content)
        if match:
            before_namespace = content[: match.start()].strip()
            namespace_content = match.group(1)
            after_namespace = content[match.end() :].strip()
            return before_namespace, namespace_content, after_namespace
        else:
            # If no redfish namespace found, assume entire content is in namespace
            return "", content, ""

    def find_function_boundaries(
        self, content: str, start_pos: int
    ) -> Tuple[int, int]:
        """Find the start and end of a function given its starting position."""
        brace_count = 0
        in_function = False
        i = start_pos

        # Find the opening brace
        while i < len(content):
            if content[i] == "{":
                if not in_function:
                    in_function = True
                    function_start = i
                brace_count += 1
            elif content[i] == "}":
                brace_count -= 1
                if in_function and brace_count == 0:
                    return function_start, i + 1
            i += 1

        return start_pos, len(content)

    def extract_functions(self, content: str) -> Tuple[List[Dict], List[Dict]]:
        """Extract inline functions and separate request routes from other functions."""
        request_routes_functions = []
        other_functions = []
        # Find all inline function matches
        for match in self.inline_function_pattern.finditer(content):
            signature = match.group(1).strip()
            function_start_pos = match.start()

            # Find the complete function
            func_start, func_end = self.find_function_boundaries(
                content, match.end() - 1
            )
            function_body = content[function_start_pos:func_end]
            # print(f"Signature: {signature}")
            # Check if this is a requestRoutes function

            if any([re.search(regex, signature) for regex in header_regexes]):
                request_routes_functions.append(
                    {
                        "signature": signature,
                        "body": function_body,
                        "start": function_start_pos,
                        "end": func_end,
                    }
                )
            else:
                other_functions.append(
                    {
                        "signature": signature,
                        "body": function_body,
                        "start": function_start_pos,
                        "end": func_end,
                    }
                )

        return request_routes_functions, other_functions

    def create_function_declaration(self, signature: str) -> str:
        """Create a function declaration from signature."""
        # Remove 'inline' and add semicolon
        declaration = re.sub(r"^\s*inline\s+", "", signature.strip())
        if not declaration.endswith(";"):
            declaration += ";"
        return declaration

    def generate_hpp_content(
        self,
        original_file: Path,
        includes: List[str],
        before_ns: str,
        after_ns: str,
        request_routes_funcs: List[Dict],
        other_funcs: List[Dict],
    ) -> str:
        """Generate the new hpp file content."""
        lines = []

        # Add license header and includes
        lines.extend(includes)
        lines.append("")

        # Add any content before namespace
        if before_ns.strip():
            lines.append(before_ns)
            lines.append("")

        # Start namespace
        lines.append("namespace redfish")
        lines.append("{")
        lines.append("")

        # Add ONLY declarations for request routes functions (not implementations)
        if request_routes_funcs:
            for func in request_routes_funcs:
                declaration = self.create_function_declaration(
                    func["signature"]
                )
                lines.append(declaration)
            lines.append("")

        # Close namespace
        lines.append("} // namespace redfish")

        return "\n".join(lines)

    def generate_cpp_content(self, original_file: Path) -> str:
        """Generate the new cpp file content."""
        with open(original_file, "r") as f:
            content = f.read()

        content = content.replace("inline", "static")
        fname = os.path.basename(original_file)
        content = content.replace("#pragma once\n", f'#include "{fname}"\n')
        for regex in header_regexes:
            content = re.sub(
                r"static ([\w:<>]+) (" + regex + r")", r"\1 \2", content
            )

        return content

    def process_hpp_file(self, hpp_file: Path) -> Tuple[str, str]:
        """Process a single hpp file and return hpp and cpp content."""

        with open(hpp_file, "r") as f:
            content = f.read()

        # Extract includes and main content
        includes, main_content = self.extract_includes(content)

        # Extract namespace content
        before_ns, namespace_content, after_ns = (
            self.extract_namespace_content(main_content)
        )

        # Extract functions
        request_routes_funcs, other_funcs = self.extract_functions(
            namespace_content
        )

        print(f"  Found {len(request_routes_funcs)} request routes functions")
        print(f"  Found {len(other_funcs)} other inline functions")

        # Generate new hpp content
        new_hpp_content = self.generate_hpp_content(
            hpp_file,
            includes,
            before_ns,
            after_ns,
            request_routes_funcs,
            other_funcs,
        )

        # Generate cpp content
        new_cpp_content = self.generate_cpp_content(hpp_file)

        return new_hpp_content, new_cpp_content

    def update_meson_build(self, new_cpp_files: List[str]):
        """Update meson.build to include new cpp files."""
        print("Updating meson.build...")

        with open(self.meson_build, "r") as f:
            content = f.read()

        # Find the srcfiles_bmcweb section
        pattern = r"(srcfiles_bmcweb\s*=\s*files\s*\([^)]*)"
        match = re.search(pattern, content, re.DOTALL)

        if not match:
            print("Warning: Could not find srcfiles_bmcweb in meson.build")
            return

        # Add new cpp files
        existing_files = match.group(1)
        new_entries = []

        for cpp_file in sorted(new_cpp_files):
            relative_path = f"'redfish-core/lib/{Path(cpp_file).name}'"
            new_entries.append(f"    {relative_path},")

        # Insert new entries before the closing parenthesis
        new_srcfiles = existing_files
        for entry in new_entries:
            new_srcfiles += f"\n{entry}"

        # Replace in content
        new_content = content.replace(match.group(1), new_srcfiles)

        # Write back
        with open(self.meson_build, "w") as f:
            f.write(new_content)

        print(f"Added {len(new_cpp_files)} cpp files to meson.build")

    def convert_all_files(self, dry_run: bool = False):
        """Convert all hpp files in redfish-core/lib."""
        if not self.redfish_core_lib.exists():
            print(f"Error: {self.redfish_core_lib} does not exist")
            return

        # Find all hpp files in lib directory
        hpp_files = list(self.redfish_core_lib.glob("*.hpp"))
        print(f"Found {len(hpp_files)} hpp files to process")

        new_cpp_files = []

        for hpp_file in hpp_files:
            print(f"Processing {hpp_file}")
            if hpp_file.name in [
                "led.hpp",
                "task.hpp",
                "redfish_util.hpp",
                "sensors.hpp",
                "network_protocol.hpp",
                "ethernet.hpp",
            ]:
                print(f"Skipping {hpp_file}")
                continue

            # Process the file
            new_hpp_content, new_cpp_content = self.process_hpp_file(hpp_file)
            if new_hpp_content == "" and new_cpp_content == "":
                continue

            # Write new hpp file
            with open(hpp_file, "w") as f:
                f.write(new_hpp_content)
            print(f"  Updated {hpp_file}")

            # Write cpp file if there's content
            if new_cpp_content.strip():
                cpp_file = self.redfish_core_lib / (hpp_file.stem + ".cpp")
                with open(cpp_file, "w") as f:
                    f.write(new_cpp_content)
                print(f"  Created {cpp_file}")
                new_cpp_files.append(str(cpp_file))
            else:
                print(f"  No cpp file needed for {hpp_file}")

        # Update meson.build
        if new_cpp_files:
            self.update_meson_build(new_cpp_files)


def main():
    parser = argparse.ArgumentParser(
        description="Convert hpp files to hpp/cpp pairs"
    )

    parser.add_argument(
        "--project-root",
        type=Path,
        default=Path.cwd(),
        help="Root directory of the bmcweb project",
    )

    args = parser.parse_args()

    if not (args.project_root / "redfish-core").exists():
        print(
            f"Error: {args.project_root} doesn't appear to be a bmcweb project root"
        )
        print("Expected to find redfish-core directory")
        sys.exit(1)

    converter = HeaderConverter(args.project_root)
    converter.convert_all_files()

    print("\nConversion completed!")


if __name__ == "__main__":
    main()
