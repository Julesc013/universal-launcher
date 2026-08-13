# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / "release" / "index" / "sdk_package_workunit.v1.toml"

EXPECTED_TARGETS = [
    "UniversalLauncher::Headers",
    "UniversalLauncher::CoreStatic",
    "UniversalLauncher::CoreShared",
]
EXPECTED_MODES = [
    "source_workspace",
    "installed_static",
    "installed_shared",
    "relocated_installed_static",
    "relocated_installed_shared",
]
FORBIDDEN_TARGETS = [
    "UniversalLauncher::Process",
    "UniversalLauncher::Execution",
    "UniversalLauncher::ReferenceStore",
    "UniversalLauncher::Daemon",
    "UniversalLauncher::ClientCpp",
]


def check_data(data: dict[str, object]) -> list[str]:
    problems: list[str] = []
    expected = {
        "schema": "universal_launcher.sdk_package_workunit.v1",
        "workunit": "ULK-CMAKE-SDK-PACKAGE-01",
        "base_ref": "refs/heads/dev",
        "base_revision": "719a3ec240831547071d69098e1fe8c76f327fb7",
        "task_branch": "task/cmake-sdk-package-01",
        "package_version": "1.8.0",
        "c_abi_major": 1,
        "c_abi_minor": 8,
        "contract_maturity": "fixture-qualified",
        "exported_targets": EXPECTED_TARGETS,
        "required_modes": EXPECTED_MODES,
    }
    for key, value in expected.items():
        if data.get(key) != value:
            problems.append(f"SDK WorkUnit {key} must be {value!r}")
    if data.get("status") not in {"active_implementation", "task_complete"}:
        problems.append("SDK WorkUnit status must be active_implementation or task_complete")

    scope = data.get("scope")
    if not isinstance(scope, dict):
        problems.append("SDK WorkUnit scope table is required")
    else:
        for key in (
            "new_launcher_behavior",
            "consumer_repin",
            "consumer_adoption",
            "setup_mutation",
            "product_execution",
            "signing",
            "publication",
            "contract_maturity_advanced",
            "usk_sdk_packaging_activated",
        ):
            if scope.get(key) is not False:
                problems.append(f"SDK WorkUnit scope.{key} must remain false")

    observation = data.get("consumer_observation")
    if not isinstance(observation, dict):
        problems.append("SDK WorkUnit consumer_observation table is required")
    else:
        if observation.get("facman_stable_ulk_pin") != "7fc25340623131ba86c08dca4fb8a43b18a4520d":
            problems.append("FacMan stable ULK pin observation changed")
        if observation.get("pin_changed") is not False:
            problems.append("consumer_observation.pin_changed must remain false")
    return problems


def check() -> list[str]:
    problems: list[str] = []
    if not RECORD.is_file():
        return ["release/index/sdk_package_workunit.v1.toml is missing"]
    with RECORD.open("rb") as handle:
        problems.extend(check_data(tomllib.load(handle)))

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(r"project\(universal_launcher\s+VERSION\s+1\.9\.0", cmake):
        problems.append("current CMake project version must be 1.9.0")
    for token in (
        "GNUInstallDirs",
        "CMakePackageConfigHelpers",
        "configure_package_config_file",
        "write_basic_package_version_file",
        "install(EXPORT UniversalLauncherTargets",
        "$<BUILD_INTERFACE:",
        "$<INSTALL_INTERFACE:",
    ):
        if token not in cmake:
            problems.append(f"CMake SDK package is missing {token}")
    for target in FORBIDDEN_TARGETS:
        if target in cmake:
            problems.append(f"aspirational SDK target must not be exported: {target}")

    maturity = (ROOT / "release" / "index" / "contract_maturity.v1.toml").read_text(encoding="utf-8")
    if 'maturity = "fixture-qualified"' not in maturity:
        problems.append("contract maturity must remain fixture-qualified")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"sdk-package-check: {problem}")
        return 1
    print("sdk-package-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
