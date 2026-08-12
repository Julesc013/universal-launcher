# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RECORD_SCHEMA = ROOT / "contracts" / "schema" / "session" / "session_record.v1.schema.json"
LIST_SCHEMA = ROOT / "contracts" / "schema" / "session" / "session_list.v1.schema.json"

FORBIDDEN_PRODUCT_TERMS = {"factorio", "steam", "modset", "save file", "scenario"}


def check() -> list[str]:
    problems: list[str] = []
    schemas: dict[Path, dict[str, object]] = {}
    for path in (RECORD_SCHEMA, LIST_SCHEMA):
        if not path.is_file():
            problems.append(f"missing session schema {path.relative_to(ROOT)}")
            continue
        try:
            schemas[path] = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            problems.append(f"invalid session schema {path.relative_to(ROOT)}: {error}")
    if problems:
        return problems

    record = schemas[RECORD_SCHEMA]
    listing = schemas[LIST_SCHEMA]
    properties = record.get("properties", {})
    required = set(record.get("required", []))
    if properties.get("schema", {}).get("const") != "ulk.session_record.v1":
        problems.append("session-record schema identity is not exact")
    for field in (
        "session_id", "operation_id", "attempt_id", "runnable_reference",
        "state", "started_at", "ended_at", "terminal_result",
        "recovery_reference", "relaunch_reference",
    ):
        if field not in required:
            problems.append(f"session-record schema must require {field}")
    if set(properties.get("state", {}).get("enum", [])) != {"running", "terminal"}:
        problems.append("session state must be exactly running and terminal")
    if record.get("additionalProperties") is not False:
        problems.append("session-record schema must reject unknown fields")
    if listing.get("properties", {}).get("schema", {}).get("const") != "ulk.session_list.v1":
        problems.append("session-list schema identity is not exact")
    if listing.get("additionalProperties") is not False:
        problems.append("session-list schema must reject unknown fields")

    combined = json.dumps([record, listing], sort_keys=True).lower()
    for anchor in (
        "operation_outcome.v1.schema.json", "recovery_reference",
        "relaunch_reference", "running", "terminal", "maxitems",
    ):
        if anchor not in combined:
            problems.append(f"session schemas are missing {anchor}")
    for forbidden in FORBIDDEN_PRODUCT_TERMS:
        if forbidden in combined:
            problems.append(f"session schemas contain product-specific term {forbidden}")

    header = (ROOT / "include" / "ulk" / "ulk_session.h").read_text(encoding="utf-8")
    source = (ROOT / "runtime" / "launcher" / "kernel" / "ulk_session.c").read_text(encoding="utf-8")
    for symbol in (
        "ulk_session_journal_write_v1", "ulk_session_journal_inspect_v1",
        "ulk_session_journal_last_run_v1", "ulk_session_journal_list_v1",
    ):
        if symbol not in header or symbol not in source:
            problems.append(f"session ABI is missing {symbol}")
    for invariant in (
        "session_operation_identity_conflict", "session_terminal_immutable",
        "session_idempotency_conflict", "session_record_incompatible",
        "session_record_corrupt", "ulk_write_file_atomic", "ulk_acquire_lock",
    ):
        if invariant not in source:
            problems.append(f"session implementation is missing invariant {invariant}")
    for forbidden in FORBIDDEN_PRODUCT_TERMS:
        if forbidden in (header + source).lower():
            problems.append(f"session ABI contains product-specific term {forbidden}")

    cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    for anchor in ("runtime/launcher/kernel/ulk_session.c", "ulk_session_journal_smoke"):
        if anchor not in cmake:
            problems.append(f"session native proof is missing {anchor}")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"session-contract-check: {problem}", file=sys.stderr)
        return 1
    print("session-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
