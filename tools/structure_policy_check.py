from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

ALLOWED_TOP_LEVEL = {
    ".github",
    ".gitignore",
    "CMakeLists.txt",
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
ALLOWED_RUNTIME_ROOTS = {"base", "client", "daemon", "launcher", "platform"}
ALLOWED_LAUNCHER_MODULES = {
    "account",
    "audit",
    "command",
    "diagnostics",
    "install_ref",
    "instance",
    "kernel",
    "launch",
    "launch_plan",
    "product",
    "profile",
}
ALLOWED_CONTRACT_ROOTS = {"abi", "command", "diagnostic", "policy", "refusal", "result", "schema"}
ALLOWED_SCHEMA_ROOTS = {"account", "command", "common", "daemon", "diagnostic", "install_ref", "instance", "launch_plan", "launcher", "product", "profile"}
ALLOWED_CONTENT_ROOTS = {"policy", "templates"}
ALLOWED_RELEASE_ROOTS = {"packaging", "profiles"}
ALLOWED_PACKAGING_ROOTS = {"linux", "macos", "portable", "windows"}
ALLOWED_APPS = {"appkit", "cli", "daemon", "gtk", "qt", "tui", "winforms"}


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
    problems.extend(check_children("release/packaging", ALLOWED_PACKAGING_ROOTS))
    problems.extend(check_children("apps", ALLOWED_APPS))
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


def check_children(relative_root: str, allowed: set[str]) -> list[str]:
    root = ROOT / relative_root
    if not root.exists():
        return [f"missing required root {relative_root}/"]
    problems: list[str] = []
    for child in root.iterdir():
        if child.name == "README.md":
            continue
        if child.is_dir() and child.name in allowed:
            continue
        problems.append(f"{relative_root}/ contains unexpected path {child.name}")
    return problems


def check_required_paths() -> list[str]:
    required = [
        ROOT / "include" / "ulk" / "ulk_api.h",
        ROOT / "include" / "ulk" / "ulk_command.h",
        ROOT / "runtime" / "launcher" / "kernel" / "ulk_api.c",
        ROOT / "contracts" / "schema" / "command" / "command_request.v1.schema.json",
        ROOT / "contracts" / "schema" / "launch_plan" / "launch_plan.v1.schema.json",
        ROOT / "docs" / "architecture" / "boundary.md",
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
