#!/usr/bin/env python3
"""Run the CLI tests declared in tests/CMakeLists.txt without CMake/CTest.

This is a release-verification fallback for environments that have a C++
compiler but no CMake installation. The CMake file remains the single source
of truth: commands and PASS_REGULAR_EXPRESSION values are parsed directly.
"""

from __future__ import annotations

import argparse
from pathlib import Path
import re
import shlex
import subprocess
import sys


ADD_TEST = re.compile(
    r"add_test\(NAME\s+(cli_[A-Za-z0-9_]+)\s+COMMAND\s+fpga_analyzer_cli\s+(.*?)\)",
    re.DOTALL,
)
PASS_REGEX = re.compile(
    r"set_tests_properties\(\s*(cli_[A-Za-z0-9_]+)\s+PROPERTIES\s+"
    r"PASS_REGULAR_EXPRESSION\s+\"((?:\\.|[^\"])*)\"\s*\)",
    re.DOTALL,
)


def cmake_unescape(value: str) -> str:
    """Decode the limited CMake quoted-string escapes used by this test file."""
    return re.sub(r"\\([\\\"])", r"\1", value)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True, type=Path,
                        help="Path to the fpga_analyzer_cli executable")
    arguments = parser.parse_args()

    tests_directory = Path(__file__).resolve().parent
    repository = tests_directory.parent
    cmake_file = tests_directory / "CMakeLists.txt"
    source = cmake_file.read_text(encoding="utf-8")
    patterns = {
        name: cmake_unescape(pattern)
        for name, pattern in PASS_REGEX.findall(source)
    }
    declarations = ADD_TEST.findall(source)
    if not declarations:
        print("No CLI test declarations found", file=sys.stderr)
        return 2

    cli = arguments.cli.resolve()
    failures: list[str] = []
    for index, (name, raw_command) in enumerate(declarations, start=1):
        expanded = raw_command.replace(
            "${CMAKE_CURRENT_SOURCE_DIR}", str(tests_directory)
        )
        command = [str(cli), *shlex.split(expanded)]
        completed = subprocess.run(
            command,
            cwd=repository,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            check=False,
        )
        output = completed.stdout
        pattern = patterns.get(name)
        reason = None
        if completed.returncode != 0:
            reason = f"exit code {completed.returncode}"
        elif pattern is None:
            reason = "missing PASS_REGULAR_EXPRESSION"
        elif re.search(pattern, output, re.DOTALL) is None:
            reason = f"output did not match {pattern!r}"
        if reason is None:
            print(f"[{index:02d}/{len(declarations):02d}] PASS {name}")
        else:
            failures.append(name)
            print(f"[{index:02d}/{len(declarations):02d}] FAIL {name}: {reason}")
            if output:
                print(output[:2000])

    print(f"CLI regressions: {len(declarations) - len(failures)} passed, "
          f"{len(failures)} failed, {len(declarations)} total")
    return 1 if failures else 0


if __name__ == "__main__":
    raise SystemExit(main())
