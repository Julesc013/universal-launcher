# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def check() -> list[str]:
    problems: list[str] = []
    header = ROOT / "include" / "ulk" / "ulk_owned_response.h"
    source = ROOT / "runtime" / "client" / "ulk_owned_response.c"
    native_test = ROOT / "tests" / "native" / "ulk_owned_response_smoke.c"
    architecture = ROOT / "docs" / "architecture" / "owned_responses.md"

    required_paths = (header, source, native_test, architecture)
    for path in required_paths:
        if not path.is_file():
            problems.append(f"missing owned-response path {path.relative_to(ROOT)}")
    if problems:
        return problems

    header_text = header.read_text(encoding="utf-8")
    source_text = source.read_text(encoding="utf-8")
    native_text = native_test.read_text(encoding="utf-8")
    architecture_text = architecture.read_text(encoding="utf-8")

    for symbol in (
        "ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1",
        "ulk_owned_command_response_v1",
        "ulk_command_response_validate_v1",
        "ulk_command_response_copy_owned_v1",
        "ulk_owned_command_response_release_v1",
    ):
        if symbol not in header_text or symbol not in source_text:
            problems.append(f"owned-response ABI is missing {symbol}")

    for invariant in (
        "ulk_owned_size_add",
        "ULK_OWNED_COMMAND_RESPONSE_BYTE_BUDGET_V1",
        "SIZE_MAX",
        "storage_size",
        "effective_allocator.alloc",
        "response->allocator.free",
        "ulk_owned_response_reset",
    ):
        if invariant not in source_text:
            problems.append(f"owned-response implementation is missing {invariant}")

    for case in (
        "test_validation",
        "test_default_copy_and_release",
        "test_allocator_and_empty_views",
        "test_invalid_allocator_and_destination",
        "test_allocation_failure",
        "test_budget_and_overflow",
        "test_context_response_sources",
    ):
        if case not in native_text:
            problems.append(f"owned-response native acceptance is missing {case}")

    for phrase in (
        "one contiguous allocation",
        "1 MiB",
        "idempotent",
        "does not make a context concurrent",
    ):
        if phrase not in architecture_text:
            problems.append(f"owned-response architecture is missing {phrase!r}")

    cmake_text = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for anchor in (
        "runtime/client/ulk_owned_response.c",
        "tests/native/ulk_owned_response_smoke.c",
        "add_test(NAME ulk_owned_response_smoke",
    ):
        if anchor not in cmake_text:
            problems.append(f"owned-response CMake wiring is missing {anchor}")

    api_text = (ROOT / "include" / "ulk" / "ulk_api.h").read_text(
        encoding="utf-8"
    )
    if '#include "ulk_owned_response.h"' not in api_text:
        problems.append("ulk_api.h must publish the owned-response ABI")
    if "ulk_command_execute_v1" not in api_text:
        problems.append("owned-response addition must preserve borrowed command execution")

    client_text = (ROOT / "include" / "ulk" / "ulk_client.h").read_text(
        encoding="utf-8"
    )
    if "ulk_client_execute_v1" not in client_text:
        problems.append("owned-response addition must preserve the borrowed client ABI")

    command_text = (ROOT / "include" / "ulk" / "ulk_command.h").read_text(
        encoding="utf-8"
    )
    for phrase in ("producing API or callback", "Registered-handler and transport"):
        if phrase not in command_text:
            problems.append(
                f"borrowed-response lifetime contract is missing {phrase!r}"
            )

    allocator_text = (ROOT / "include" / "ulk" / "ulk_allocator.h").read_text(
        encoding="utf-8"
    )
    for declaration in (
        "ULK_CALL *ulk_alloc_fn_v1",
        "ULK_CALL *ulk_free_fn_v1",
    ):
        if declaration not in allocator_text:
            problems.append(
                f"allocator callback convention is missing {declaration!r}"
            )

    types_text = (ROOT / "include" / "ulk" / "ulk_types.h").read_text(
        encoding="utf-8"
    )
    major = re.search(r"#define ULK_API_VERSION_MAJOR (\d+)", types_text)
    minor = re.search(r"#define ULK_API_VERSION_MINOR (\d+)", types_text)
    if major is None or int(major.group(1)) != 1:
        problems.append("owned-response ABI must preserve ULK ABI major 1")
    if minor is None or int(minor.group(1)) < 7:
        problems.append("owned-response ABI requires ULK ABI minor 7 or later")

    combined = f"{header_text}\n{source_text}".lower()
    for forbidden in ("factorio", "modset", "save file", "setup mutation"):
        if forbidden in combined:
            problems.append(
                f"owned-response ABI contains product or setup term {forbidden}"
            )
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"owned-response-contract-check: {problem}", file=sys.stderr)
        return 1
    print("owned-response-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
