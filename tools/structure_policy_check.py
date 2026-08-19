# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

ALLOWED_TOP_LEVEL = {
    ".github",
    ".gitattributes",
    ".gitignore",
    "CMakeLists.txt",
    "LICENSE",
    "README.md",
    "apps",
    "archive",
    "cmake",
    "content",
    "contracts",
    "docs",
    "include",
    "release",
    "runtime",
    "tests",
    "tools",
}

IGNORED_TOP_LEVEL = {".git", "__pycache__", ".pytest_cache", "build", "dist", "out", "bin", "obj"}
RETIRED_ROOTS = {"core", "data", "factorio", "packages", "packaging", "schema", "schemas", "setup", "source", "src", "ui"}
ALLOWED_RUNTIME_ROOTS = {"base", "binding", "client", "daemon", "launcher", "platform"}
ALLOWED_LAUNCHER_MODULES = {
    "account",
    "artifact_set",
    "audit",
    "command",
    "compatibility",
    "discovery",
    "diagnostics",
    "export",
    "install_ref",
    "instance",
    "kernel",
    "launch_plan",
    "product",
    "profile",
}
ALLOWED_CONTRACT_ROOTS = {"abi", "command", "diagnostic", "policy", "refusal", "result", "schema"}
ALLOWED_SCHEMA_ROOTS = {
    "account_ref",
    "artifact_set",
    "command",
    "common",
    "composition",
    "daemon",
    "diagnostic",
    "install_ref",
    "instance",
    "launch_plan",
    "launcher",
    "package",
    "product",
    "profile",
    "reference",
    "session",
    "setup",
}
ALLOWED_CONTENT_ROOTS = {"policy", "templates"}
ALLOWED_RELEASE_ROOTS = {"index", "license.v1.toml", "packaging", "profiles"}
ALLOWED_RELEASE_INDEX = {
    "branch_policy.v1.toml",
    "consumer_matrix.v1.toml",
    "contract_maturity.v1.toml",
    "incubator_intake.v1.toml",
    "provider_capabilities.v1.toml",
    "sdk_package_workunit.v1.toml",
    "session_last_run_promotion.v1.toml",
}
ALLOWED_PACKAGING_ROOTS = {"bsd", "linux", "macos", "portable", "windows"}
ALLOWED_APPS = {"cli", "daemon", "gui", "tui"}


def main() -> int:
    problems: list[str] = []
    problems.extend(check_top_level())
    problems.extend(check_retired_roots())
    problems.extend(check_no_src_source_dirs())
    problems.extend(check_children("runtime", ALLOWED_RUNTIME_ROOTS))
    problems.extend(check_children("runtime/launcher", ALLOWED_LAUNCHER_MODULES))
    problems.extend(check_children("contracts", ALLOWED_CONTRACT_ROOTS))
    problems.extend(check_children("contracts/schema", ALLOWED_SCHEMA_ROOTS))
    problems.extend(check_children("content", ALLOWED_CONTENT_ROOTS))
    problems.extend(check_children("release", ALLOWED_RELEASE_ROOTS))
    problems.extend(check_children("release/index", ALLOWED_RELEASE_INDEX))
    problems.extend(check_children("release/packaging", ALLOWED_PACKAGING_ROOTS))
    problems.extend(check_children("apps", ALLOWED_APPS))
    problems.extend(check_children("apps/gui", set()))
    problems.extend(check_no_language_version_runtime_buckets())
    problems.extend(check_required_paths())
    problems.extend(check_forbidden_product_or_setup_semantics())
    if problems:
        for problem in problems:
            print(f"structure-check: {problem}", file=sys.stderr)
        return 1
    print("structure-check: ok")
    return 0


def check_top_level() -> list[str]:
    problems: list[str] = []
    for path in ROOT.iterdir():
        if path.name in IGNORED_TOP_LEVEL:
            continue
        if path.name not in ALLOWED_TOP_LEVEL:
            problems.append(f"unexpected top-level path {path.name}")
    return problems


def check_retired_roots() -> list[str]:
    return [f"retired or forbidden root must not exist: {name}/" for name in sorted(RETIRED_ROOTS) if (ROOT / name).exists()]


def check_no_src_source_dirs() -> list[str]:
    problems: list[str] = []
    for path in ROOT.rglob("*"):
        if path.is_dir() and path.name in {"src", "source"}:
            problems.append(f"forbidden implementation bucket: {path.relative_to(ROOT)}")
    return problems


def check_no_language_version_runtime_buckets() -> list[str]:
    problems: list[str] = []
    forbidden = {"c11", "c17", "cpp98", "cpp11", "cpp17", "cxx98", "cxx11", "cxx17"}
    for path in (ROOT / "runtime").rglob("*"):
        if path.is_dir() and path.name.lower() in forbidden:
            problems.append(f"runtime folders must be domain-based, not language-version buckets: {path.relative_to(ROOT)}")
    return problems


def check_children(relative_root: str, allowed: set[str]) -> list[str]:
    root = ROOT / relative_root
    if not root.exists():
        return [f"missing required root {relative_root}/"]
    problems: list[str] = []
    for child in root.iterdir():
        if child.name == "README.md":
            continue
        if child.name in allowed:
            continue
        problems.append(f"{relative_root}/ contains unexpected path {child.name}")
    return problems


def check_required_paths() -> list[str]:
    required = [
        ROOT / "include" / "ulk" / "ulk_api.h",
        ROOT / "include" / "ulk" / "ulk_command.h",
        ROOT / "include" / "ulk" / "ulk_client.h",
        ROOT / "include" / "ulk" / "ulk_operation.h",
        ROOT / "include" / "ulk" / "ulk_session.h",
        ROOT / "include" / "ulk" / "ulk_owned_response.h",
        ROOT / "include" / "ulk" / "ulk_product_composition.h",
        ROOT / "include" / "ulk" / "ulk_reference_model.h",
        ROOT / "include" / "ulk" / "ulk_artifact_set.h",
        ROOT / "include" / "ulu" / "ulu_api.h",
        ROOT / "runtime" / "launcher" / "kernel" / "ulk_api.c",
        ROOT / "runtime" / "client" / "ulk_client.c",
        ROOT / "runtime" / "client" / "ulk_operation.c",
        ROOT / "runtime" / "client" / "ulk_owned_response.c",
        ROOT / "runtime" / "launcher" / "kernel" / "ulk_product_composition.c",
        ROOT / "runtime" / "launcher" / "kernel" / "ulk_reference_model.c",
        ROOT / "runtime" / "launcher" / "kernel" / "ulk_session.c",
        ROOT / "contracts" / "abi" / "ulk_c_abi.v1.toml",
        ROOT / "runtime" / "launcher" / "artifact_set" / "README.md",
        ROOT / "contracts" / "schema" / "command" / "command_request.v1.schema.json",
        ROOT / "contracts" / "schema" / "command" / "client_transport.v1.schema.json",
        ROOT / "contracts" / "schema" / "command" / "operation_outcome.v1.schema.json",
        ROOT / "contracts" / "schema" / "reference" / "reference_graph.v1.schema.json",
        ROOT / "contracts" / "schema" / "artifact_set" / "artifact_set.v1.schema.json",
        ROOT / "contracts" / "schema" / "launch_plan" / "launch_plan.v1.schema.json",
        ROOT / "contracts" / "schema" / "composition" / "product_composition.v1.schema.json",
        ROOT / "contracts" / "schema" / "session" / "session_record.v1.schema.json",
        ROOT / "contracts" / "schema" / "session" / "session_list.v1.schema.json",
        ROOT / "docs" / "architecture" / "boundary.md",
        ROOT / "docs" / "architecture" / "ecosystem_vision.md",
        ROOT / "docs" / "architecture" / "reference_model.md",
        ROOT / "docs" / "architecture" / "owned_responses.md",
        ROOT / "docs" / "architecture" / "product_composition.md",
        ROOT / "docs" / "architecture" / "session_journal.md",
        ROOT / "docs" / "architecture" / "root_grammar.md",
        ROOT / "docs" / "roadmap.md",
        ROOT / "docs" / "governance" / "branch_model.md",
        ROOT / "tests" / "compat" / "ulk_abi_1_6" / "ulk_client_api_1_6.h",
        ROOT / "tests" / "native" / "ulk_abi_1_6_client_compat_smoke.c",
        ROOT / "tests" / "native" / "ulk_abi_layout_smoke.c",
        ROOT / "tests" / "native" / "ulk_owned_response_export_smoke.c",
        ROOT / "tests" / "native" / "ulk_product_composition_smoke.c",
        ROOT / "tests" / "native" / "ulk_session_journal_smoke.c",
        ROOT / "tools" / "abi_contract_check.py",
        ROOT / "tools" / "product_composition_contract_check.py",
        ROOT / "tools" / "session_contract_check.py",
        ROOT / "tools" / "branch_policy_check.py",
        ROOT / "release" / "index" / "branch_policy.v1.toml",
        ROOT / "release" / "index" / "contract_maturity.v1.toml",
        ROOT / "release" / "index" / "sdk_package_workunit.v1.toml",
        ROOT / "release" / "index" / "session_last_run_promotion.v1.toml",
        ROOT / "cmake" / "UniversalLauncherConfig.cmake.in",
        ROOT / "cmake" / "README-SDK.md",
        ROOT / "tests" / "sdk" / "consumer" / "CMakeLists.txt",
        ROOT / "tests" / "sdk" / "consumer" / "main.c",
        ROOT / "tests" / "sdk" / "consumer" / "cpp_headers.cpp",
        ROOT / "tests" / "sdk" / "embedded" / "CMakeLists.txt",
        ROOT / "tools" / "cmake_sdk_conformance.py",
        ROOT / "tools" / "sdk_package_check.py",
        ROOT / "tools" / "session_promotion_check.py",
    ]
    return [f"missing required path {path.relative_to(ROOT)}" for path in required if not path.exists()]


def check_forbidden_product_or_setup_semantics() -> list[str]:
    forbidden = [
        "factorio",
        "mod_portal",
        "modset",
        "save_manager",
        "server_manager",
        "rollback",
        "uninstall",
        "repair_plan",
    ]
    problems: list[str] = []
    for path in ROOT.rglob("*"):
        rel = path.relative_to(ROOT).as_posix().lower()
        if rel.startswith(".git/"):
            continue
        for token in forbidden:
            if token in rel:
                problems.append(f"universal-launcher must not contain product/setup semantic path {path.relative_to(ROOT)}")
    return problems


if __name__ == "__main__":
    raise SystemExit(main())
