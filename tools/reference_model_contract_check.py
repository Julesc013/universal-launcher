# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_ROOT = ROOT / "contracts" / "schema"
REFERENCE_ROOT = SCHEMA_ROOT / "reference"

REQUIRED_SCHEMAS = {
    "product_reference.v1.schema.json": "ulk.product_reference.v1",
    "install_reference.v2.schema.json": "ulk.install_reference.v2",
    "instance_reference.v2.schema.json": "ulk.instance_reference.v2",
    "profile_reference.v2.schema.json": "ulk.profile_reference.v2",
    "artifact_set_reference.v1.schema.json": "ulk.artifact_set_reference.v1",
    "launch_plan_reference.v2.schema.json": "ulk.launch_plan_reference.v2",
    "reference_graph.v1.schema.json": "ulk.reference_graph.v1",
}

FORBIDDEN_PRODUCT_TERMS = {
    "factorio",
    "steam",
    "modset",
    "save file",
    "scenario",
}


def local_refs(value: object) -> list[str]:
    refs: list[str] = []
    if isinstance(value, dict):
        for key, child in value.items():
            if key == "$ref" and isinstance(child, str) and "://" not in child:
                refs.append(child)
            else:
                refs.extend(local_refs(child))
    elif isinstance(value, list):
        for child in value:
            refs.extend(local_refs(child))
    return refs


def check() -> list[str]:
    problems: list[str] = []
    for filename, schema_name in REQUIRED_SCHEMAS.items():
        path = REFERENCE_ROOT / filename
        if not path.is_file():
            problems.append(f"missing reference-model schema {path.relative_to(ROOT)}")
            continue
        try:
            schema = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            problems.append(f"invalid JSON in {path.relative_to(ROOT)}: {error}")
            continue
        properties = schema.get("properties", {})
        if properties.get("schema", {}).get("const") != schema_name:
            problems.append(f"{filename} does not bind {schema_name}")
        if schema.get("additionalProperties") is not False:
            problems.append(f"{filename} must reject unknown fields")
        schema_text = json.dumps(schema, sort_keys=True).lower()
        for forbidden in FORBIDDEN_PRODUCT_TERMS:
            if forbidden in schema_text:
                problems.append(f"{filename} contains product-specific term {forbidden}")
        for ref in local_refs(schema):
            reference_path = unquote(ref.split("#", 1)[0])
            if not reference_path:
                continue
            target = (path.parent / reference_path).resolve()
            if not target.is_relative_to(SCHEMA_ROOT.resolve()) or not target.is_file():
                problems.append(f"{filename} has unresolved local ref {ref}")

    graph_path = REFERENCE_ROOT / "reference_graph.v1.schema.json"
    if graph_path.is_file():
        graph = json.loads(graph_path.read_text(encoding="utf-8"))
        required = graph.get("required", [])
        for field in ("product", "install_reference", "instance", "launch_plan"):
            if field not in required:
                problems.append(f"reference graph must require {field}")

    header = ROOT / "include" / "ulk" / "ulk_reference_model.h"
    source = ROOT / "runtime" / "launcher" / "kernel" / "ulk_reference_model.c"
    if not header.is_file() or not source.is_file():
        problems.append("reference-model ABI header and implementation must exist")
    else:
        header_text = header.read_text(encoding="utf-8")
        source_text = source.read_text(encoding="utf-8")
        for symbol in (
            "ulk_product_ref_validate_v1",
            "ulk_install_reference_validate_v2",
            "ulk_instance_ref_validate_v2",
            "ulk_profile_ref_validate_v2",
            "ulk_artifact_set_ref_validate_v1",
            "ulk_launch_plan_ref_validate_v2",
            "ulk_reference_graph_validate_v1",
        ):
            if symbol not in header_text or symbol not in source_text:
                problems.append(f"reference-model ABI is missing {symbol}")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"reference-model-contract-check: {problem}", file=sys.stderr)
        return 1
    print("reference-model-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
