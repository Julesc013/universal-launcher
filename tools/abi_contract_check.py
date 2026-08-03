# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "contracts" / "abi" / "ulk_c_abi.v1.toml"
COMPAT_HEADER = (
    ROOT
    / "tests"
    / "compat"
    / "ulk_abi_1_6"
    / "ulk_client_api_1_6.h"
)


def _public_headers() -> list[Path]:
    return sorted((ROOT / "include" / "ulk").glob("*.h"))


def _exported_symbols(headers: list[Path]) -> list[str]:
    symbols: set[str] = set()
    for header in headers:
        text = "\n".join(
            line
            for line in header.read_text(encoding="utf-8").splitlines()
            if not line.lstrip().startswith("#")
        )
        symbols.update(
            re.findall(
                r"\bULK_API\b[^;]*?\bULK_CALL\s+([A-Za-z_]\w*)\s*\(",
                text,
                flags=re.DOTALL,
            )
        )
    return sorted(symbols)


def _normalized_fields(body: str) -> list[str]:
    body = re.sub(r"/\*.*?\*/", " ", body, flags=re.DOTALL)
    fields: list[str] = []
    for declaration in body.split(";"):
        normalized = " ".join(declaration.split())
        if normalized:
            fields.append(normalized)
    return fields


def _struct_layouts(headers: list[Path]) -> dict[str, list[str]]:
    layouts: dict[str, list[str]] = {}
    pattern = re.compile(
        r"typedef\s+struct\s+([A-Za-z_]\w*)\s*\{(.*?)\}\s*([A-Za-z_]\w*)\s*;",
        flags=re.DOTALL,
    )
    for header in headers:
        text = header.read_text(encoding="utf-8")
        for match in pattern.finditer(text):
            tag, body, alias = match.groups()
            if tag == alias:
                layouts[alias] = _normalized_fields(body)
    return layouts


def check() -> list[str]:
    problems: list[str] = []
    if not MANIFEST.is_file():
        return [f"missing ABI snapshot {MANIFEST.relative_to(ROOT)}"]
    if not COMPAT_HEADER.is_file():
        return [f"missing frozen ABI 1.6 header {COMPAT_HEADER.relative_to(ROOT)}"]

    with MANIFEST.open("rb") as handle:
        manifest = tomllib.load(handle)
    if manifest.get("schema") != "ulk.c_abi_snapshot.v1":
        problems.append("ABI snapshot schema must be ulk.c_abi_snapshot.v1")
    if manifest.get("compatibility_base_sha") != (
        "7f4312faf2f1ac2856a51393fef0ec49fc276a78"
    ):
        problems.append("ABI snapshot must bind the reviewed ABI 1.6 base")

    types_text = (ROOT / "include" / "ulk" / "ulk_types.h").read_text(
        encoding="utf-8"
    )
    major = re.search(r"#define ULK_API_VERSION_MAJOR (\d+)", types_text)
    minor = re.search(r"#define ULK_API_VERSION_MINOR (\d+)", types_text)
    current_version = (
        int(major.group(1)) if major else None,
        int(minor.group(1)) if minor else None,
    )
    snapshot_version = (manifest.get("abi_major"), manifest.get("abi_minor"))
    if current_version != (1, 7) or snapshot_version != current_version:
        problems.append(
            f"ABI version snapshot {snapshot_version!r} does not match current {current_version!r}"
        )

    headers = _public_headers()
    expected_exports = manifest.get("exports")
    if not isinstance(expected_exports, list):
        problems.append("ABI snapshot exports must be an array")
    else:
        expected = sorted(str(symbol) for symbol in expected_exports)
        if len(expected) != len(set(expected)):
            problems.append("ABI snapshot exports must be unique")
        actual = _exported_symbols(headers)
        if actual != expected:
            problems.append(
                f"public export snapshot mismatch: expected {expected!r}, actual {actual!r}"
            )

    actual_layouts = _struct_layouts(headers)
    expected_layouts = manifest.get("layouts")
    if not isinstance(expected_layouts, dict):
        problems.append("ABI snapshot layouts must be tables")
    else:
        for name, snapshot in expected_layouts.items():
            expected_fields = snapshot.get("fields") if isinstance(snapshot, dict) else None
            if not isinstance(expected_fields, list):
                problems.append(f"ABI layout {name} must declare a fields array")
                continue
            actual_fields = actual_layouts.get(name)
            if actual_fields != expected_fields:
                problems.append(
                    f"ABI layout {name} mismatch: expected {expected_fields!r}, "
                    f"actual {actual_fields!r}"
                )

    compat_text = COMPAT_HEADER.read_text(encoding="utf-8")
    if "#define ULK_API_VERSION_MINOR 6" not in compat_text:
        problems.append("frozen compatibility header must remain at ABI 1.6")
    if "owned_command_response" in compat_text:
        problems.append("frozen ABI 1.6 header must not expose owned responses")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for anchor in (
        "ulk_abi_layout_smoke",
        "ulk_abi_1_6_client_compat_smoke",
        "ulk_owned_response_export_smoke",
        "target_link_libraries(ulk_abi_1_6_client_compat_smoke PRIVATE ulk_shared)",
        "target_link_libraries(ulk_owned_response_export_smoke PRIVATE ulk_shared)",
    ):
        if anchor not in cmake:
            problems.append(f"CMake ABI proof is missing {anchor}")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"abi-contract-check: {problem}", file=sys.stderr)
        return 1
    print("abi-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
