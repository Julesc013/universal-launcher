# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import re
import tomllib
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
RECORD = ROOT / "release" / "index" / "session_last_run_promotion.v1.toml"

EXPECTED_PROOFS = [
    "fresh_source_reconstruction",
    "abi_manifest_and_symbol_additivity",
    "native_session_fault_tck",
    "installed_workspace_consumer",
    "installed_static_consumer",
    "installed_shared_consumer",
    "installed_combined_consumer",
    "relocated_static_consumer",
    "relocated_shared_consumer",
    "interruption_and_corruption_recovery",
]


def check_data(data: dict[str, Any]) -> list[str]:
    problems: list[str] = []
    expected = {
        "schema": "universal_launcher.session_last_run_promotion.v1",
        "workunit": "ULK-SESSION-LAST-RUN-PROMOTION-01",
        "status": "qualified_task_candidate_pending_dev_integration",
        "reviewed_on": "2026-08-13",
        "task_branch": "task/ulk-session-last-run-promotion-01",
        "qualified_session_revision": "85df03b292c09a004352b5e66cc6fc4d9fabae51",
        "qualified_session_tree": "b3b20261bf5e5cefa64c44e52d03cc5950295afc",
        "session_implementation_revision": "6a6e49336d66fb95ce6efce90d480d60f93e66ba",
        "canonical_main_before": "1cafe4054297cc11e02458b83d230db0cd064471",
        "canonical_main_before_tree": "47018102de4b9fd20af9f77acd4e1e35e51590f3",
        "promotion_source_ref": "refs/heads/dev",
        "promotion_target_ref": "refs/heads/main",
        "package_version": "1.9.0",
        "c_abi_major": 1,
        "c_abi_minor": 9,
        "implementation_state": "implemented",
        "consumer_qualification": "qualified_input_for_facman_adoption_train",
        "public_maturity": "experimental_prerelease",
        "provider_spi_scope": "bounded_session_and_last_run_subset_not_broad_final_spi",
        "required_proofs": EXPECTED_PROOFS,
    }
    for field, value in expected.items():
        if data.get(field) != value:
            problems.append(f"session promotion {field} must be {value!r}")

    compatibility = data.get("compatibility", {})
    for field in (
        "main_before_is_ancestor_of_qualified_session",
        "abi_major_preserved",
        "abi_minor_additive",
        "frozen_abi_1_6_compatibility_preserved",
        "borrowed_client_abi_preserved",
        "owned_response_abi_preserved",
        "product_composition_abi_preserved",
        "setup_handoff_abi_preserved",
    ):
        if compatibility.get(field) is not True:
            problems.append(f"session promotion compatibility.{field} must be true")
    if compatibility.get("migration_review") != "no_existing_session_journal_state_to_migrate":
        problems.append("session promotion migration review must remain explicit")

    qualification = data.get("qualification", {})
    expected_qualification = {
        "strict_validation": "pass",
        "python_tests": 17,
        "native_tests": 15,
        "hosted_pull_request_checks": 8,
        "hosted_merge_head_checks": 1,
        "required_skips": 0,
        "installed_sdk_modes": ["workspace", "static", "shared", "combined"],
        "relocated_sdk_modes": ["static", "shared"],
    }
    for field, value in expected_qualification.items():
        if qualification.get(field) != value:
            problems.append(f"session promotion qualification.{field} must be {value!r}")

    scope = data.get("scope", {})
    if not scope:
        problems.append("session promotion scope table is required")
    for field, value in scope.items():
        if value is not False:
            problems.append(f"session promotion scope.{field} must remain false")
    required_closed = {
        "process_execution",
        "process_supervision",
        "daemon",
        "dynamic_providers",
        "reference_store_expansion",
        "setup_mutation",
        "consumer_repin",
        "consumer_adoption",
        "factorio_terminology",
        "signing",
        "publication",
        "release_creation",
        "stable_public_spi",
    }
    if set(scope) != required_closed:
        problems.append("session promotion scope must enumerate the exact closed authorities")
    return problems


def check() -> list[str]:
    if not RECORD.is_file():
        return ["session promotion record is missing"]
    with RECORD.open("rb") as handle:
        data = tomllib.load(handle)
    problems = check_data(data)

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    if not re.search(r"project\(universal_launcher\s+VERSION\s+1\.9\.0", cmake):
        problems.append("promotion candidate package version must remain 1.9.0")
    abi = (ROOT / "contracts" / "abi" / "ulk_c_abi.v1.toml").read_text(
        encoding="utf-8"
    )
    for anchor in (
        'abi_major = 1',
        'abi_minor = 9',
        '"ulk_session_journal_write_v1"',
        '"ulk_session_journal_last_run_v1"',
    ):
        if anchor not in abi:
            problems.append(f"promotion ABI manifest is missing {anchor}")
    header = (ROOT / "include" / "ulk" / "ulk_session.h").read_text(encoding="utf-8")
    for symbol in (
        "ulk_session_journal_write_v1",
        "ulk_session_journal_inspect_v1",
        "ulk_session_journal_last_run_v1",
        "ulk_session_journal_list_v1",
    ):
        if symbol not in header:
            problems.append(f"promotion session header is missing {symbol}")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"session-promotion-check: {problem}")
        return 1
    print("session-promotion-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
