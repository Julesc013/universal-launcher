# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_PATH = (
    ROOT
    / "contracts"
    / "schema"
    / "command"
    / "operation_outcome.v1.schema.json"
)

OUTCOMES = {
    "cancelled_before_dispatch",
    "refused_before_effects",
    "completed",
    "cancellation_requested_but_completed",
    "recovery_required",
    "outcome_unknown",
}

FORBIDDEN_PRODUCT_TERMS = {
    "factorio",
    "steam",
    "modset",
    "save file",
    "scenario",
}


def check() -> list[str]:
    problems: list[str] = []
    if not SCHEMA_PATH.is_file():
        return [f"missing operation-outcome schema {SCHEMA_PATH.relative_to(ROOT)}"]
    try:
        schema = json.loads(SCHEMA_PATH.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        return [f"invalid operation-outcome schema JSON: {error}"]

    properties = schema.get("properties", {})
    if properties.get("schema", {}).get("const") != "ulk.operation_outcome.v1":
        problems.append("operation-outcome schema identity is not exact")
    if set(properties.get("outcome", {}).get("enum", [])) != OUTCOMES:
        problems.append("operation-outcome vocabulary is incomplete or widened")
    required = set(schema.get("required", []))
    for field in (
        "schema",
        "operation_id",
        "attempt_id",
        "outcome",
        "effects_may_have_occurred",
        "recovery",
    ):
        if field not in required:
            problems.append(f"operation-outcome schema must require {field}")
    recovery = properties.get("recovery", {})
    if schema.get("additionalProperties") is not False:
        problems.append("operation-outcome schema must reject unknown fields")
    if recovery.get("additionalProperties") is not False:
        problems.append("operation recovery reference must reject unknown fields")
    for field in ("required", "transaction_id", "inspect_command"):
        if field not in recovery.get("required", []):
            problems.append(f"operation recovery reference must require {field}")

    schema_text = json.dumps(schema, sort_keys=True).lower()
    for outcome in OUTCOMES:
        if outcome not in schema_text:
            problems.append(f"operation-outcome schema is missing invariant for {outcome}")
    for anchor in (
        '"const": true',
        '"const": false',
        '"minlength": 1',
        "effects_may_have_occurred",
        "inspect_command",
    ):
        if anchor not in schema_text:
            problems.append(f"operation-outcome schema is missing fail-closed anchor {anchor}")
    for forbidden in FORBIDDEN_PRODUCT_TERMS:
        if forbidden in schema_text:
            problems.append(f"operation-outcome schema contains product-specific term {forbidden}")

    header = ROOT / "include" / "ulk" / "ulk_operation.h"
    source = ROOT / "runtime" / "client" / "ulk_operation.c"
    if not header.is_file() or not source.is_file():
        problems.append("operation-outcome ABI header and implementation must exist")
    else:
        header_text = header.read_text(encoding="utf-8")
        source_text = source.read_text(encoding="utf-8")
        for symbol in (
            "ulk_operation_identity_v1",
            "ulk_operation_recovery_v1",
            "ulk_operation_result_v1",
        ):
            if symbol not in header_text:
                problems.append(f"operation-outcome ABI header is missing {symbol}")
        for symbol in (
            "ulk_operation_identity_validate_v1",
            "ulk_operation_result_validate_v1",
            "ulk_operation_outcome_name_v1",
        ):
            if symbol not in header_text or symbol not in source_text:
                problems.append(f"operation-outcome ABI is missing {symbol}")
        for invariant in (
            "pre_effect_outcome",
            "uncertain_outcome",
            "effects_may_have_occurred != 1",
            "recovery.required != 1",
            "ulk_command_name_is_valid",
        ):
            if invariant not in source_text:
                problems.append(f"operation-outcome ABI is missing invariant {invariant}")

    types = (ROOT / "include" / "ulk" / "ulk_types.h").read_text(encoding="utf-8")
    if "#define ULK_API_VERSION_MAJOR 1" not in types:
        problems.append("operation-outcome contract must preserve ULK ABI major 1")
    if "#define ULK_API_VERSION_MINOR 6" not in types:
        problems.append("operation-outcome contract must publish ULK ABI minor 6")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"operation-outcome-contract-check: {problem}", file=sys.stderr)
        return 1
    print("operation-outcome-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
