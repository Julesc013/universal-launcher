# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import json
import sys
import tomllib
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_ROOT = ROOT / "contracts" / "schema"
COMPOSITION_ROOT = SCHEMA_ROOT / "composition"
FIXTURE = ROOT / "tests" / "fixtures" / "product-composition" / "neutral-product.v1.json"
MATURITY = ROOT / "release" / "index" / "contract_maturity.v1.toml"

REQUIRED_SCHEMAS = {
    "launch_capability.v1.schema.json": "ulk.launch_capability.v1",
    "entrypoint_descriptor.v1.schema.json": "ulk.entrypoint.v1",
    "product_descriptor.v2.schema.json": "ulk.product_descriptor.v2",
    "contract_set_identity.v1.schema.json": "ulk.contract_set_identity.v1",
    "product_composition.v1.schema.json": "ulk.product_composition.v1",
}
CAPABILITIES = {
    "single_process",
    "open_document",
    "multi_instance",
    "profile_selection",
    "artifact_sets",
    "session_supervision",
    "background_service",
    "server",
}
FORBIDDEN_TERMS = {"game", "catalogue", "simulation", "factorio", "dominium", "c3"}
EXPECTED_MATURITY = {
    "product_descriptor.v2",
    "entrypoint_descriptor.v1",
    "launch_capability.v1",
    "product_composition.v1",
    "contract_set_identity.v1",
}


def _local_refs(value: object) -> list[str]:
    refs: list[str] = []
    if isinstance(value, dict):
        for key, child in value.items():
            if key == "$ref" and isinstance(child, str) and "://" not in child:
                refs.append(child)
            else:
                refs.extend(_local_refs(child))
    elif isinstance(value, list):
        for child in value:
            refs.extend(_local_refs(child))
    return refs


def check() -> list[str]:
    problems: list[str] = []
    schemas: dict[str, object] = {}
    for filename, identity in REQUIRED_SCHEMAS.items():
        path = COMPOSITION_ROOT / filename
        if not path.is_file():
            problems.append(f"missing composition schema {path.relative_to(ROOT)}")
            continue
        try:
            schema = json.loads(path.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError) as error:
            problems.append(f"invalid JSON in {path.relative_to(ROOT)}: {error}")
            continue
        schemas[filename] = schema
        if schema.get("properties", {}).get("schema", {}).get("const") != identity:
            problems.append(f"{filename} does not bind {identity}")
        if schema.get("additionalProperties") is not False:
            problems.append(f"{filename} must reject unknown fields")
        text = json.dumps(schema, sort_keys=True).lower()
        for term in FORBIDDEN_TERMS:
            if term in text:
                problems.append(f"{filename} contains forbidden product term {term}")
        for ref in _local_refs(schema):
            reference_path = unquote(ref.split("#", 1)[0])
            if not reference_path:
                continue
            target = (path.parent / reference_path).resolve()
            if not target.is_relative_to(SCHEMA_ROOT.resolve()) or not target.is_file():
                problems.append(f"{filename} has unresolved local ref {ref}")

    capability_schema = schemas.get("launch_capability.v1.schema.json", {})
    actual_capabilities = set(
        capability_schema.get("properties", {}).get("kind", {}).get("enum", [])
    )
    if actual_capabilities != CAPABILITIES:
        problems.append("launch capability vocabulary must match the ratified closed set")

    try:
        fixture = json.loads(FIXTURE.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        problems.append(f"neutral composition fixture is unavailable or invalid: {error}")
        fixture = {}
    if fixture.get("schema") != "ulk.product_composition.v1":
        problems.append("neutral fixture must bind the product-composition schema")
    if fixture.get("product", {}).get("product_id") != "org.example.fixture":
        problems.append("neutral fixture must use org.example.fixture")
    entrypoints = fixture.get("entrypoints", [])
    if len(entrypoints) != 1 or entrypoints[0].get("relative_path") != "bin/fixture":
        problems.append("neutral fixture must expose one bin/fixture entrypoint")
    kinds = {
        capability.get("kind")
        for entrypoint in entrypoints
        for capability in entrypoint.get("capabilities", [])
    }
    if kinds != {"single_process"}:
        problems.append("neutral fixture must exercise only single_process")

    for relative in (
        Path("include/ulk/ulk_product_composition.h"),
        Path("runtime/launcher/kernel/ulk_product_composition.c"),
    ):
        text = (ROOT / relative).read_text(encoding="utf-8").lower()
        for term in FORBIDDEN_TERMS:
            if term in text:
                problems.append(f"{relative} contains forbidden product term {term}")

    try:
        with MATURITY.open("rb") as handle:
            maturity = tomllib.load(handle)
    except (OSError, tomllib.TOMLDecodeError) as error:
        problems.append(f"contract maturity record is unavailable or invalid: {error}")
        maturity = {}
    contracts = maturity.get("contract", [])
    observed = {
        contract.get("id")
        for contract in contracts
        if contract.get("maturity") == "fixture-qualified" and contract.get("evidence")
    }
    if maturity.get("provider") != "universal-launcher" or observed != EXPECTED_MATURITY:
        problems.append("composition contracts must be recorded individually as fixture-qualified")
    return problems


def main() -> int:
    problems = check()
    if problems:
        for problem in problems:
            print(f"product-composition-contract-check: {problem}", file=sys.stderr)
        return 1
    print("product-composition-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
