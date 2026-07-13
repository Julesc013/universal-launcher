from __future__ import annotations

import json
import sys
from pathlib import Path
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_ROOT = ROOT / "contracts" / "schema"
SETUP_ROOT = SCHEMA_ROOT / "setup"

REQUIRED_SCHEMAS = {
    "setup_request.v1.schema.json": "ulk.setup_request.v1",
    "setup_plan_reference.v1.schema.json": "ulk.setup_plan_reference.v1",
    "setup_result.v1.schema.json": "ulk.setup_result.v1",
    "installed_state_reference.v1.schema.json": "ulk.installed_state_reference.v1",
    "install_reference_refresh.v1.schema.json": "ulk.install_reference_refresh.v1",
    "setup_authority_status.v1.schema.json": "ulk.setup_authority_status.v1",
}

FORBIDDEN_PRODUCT_TERMS = {
    "factorio",
    "steam",
    "space age",
    "mods",
    "saves",
    "app_id",
}


def load_schema(path: Path) -> dict[str, object]:
    return json.loads(path.read_text(encoding="utf-8"))


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


def main() -> int:
    problems: list[str] = []
    for filename, schema_name in REQUIRED_SCHEMAS.items():
        path = SETUP_ROOT / filename
        if not path.is_file():
            problems.append(f"missing setup handoff schema {path.relative_to(ROOT)}")
            continue
        try:
            schema = load_schema(path)
        except (OSError, json.JSONDecodeError) as error:
            problems.append(f"invalid JSON in {path.relative_to(ROOT)}: {error}")
            continue
        schema_text = json.dumps(schema, sort_keys=True).lower()
        if schema.get("properties", {}).get("schema", {}).get("const") != schema_name:
            problems.append(f"{filename} does not bind {schema_name}")
        for forbidden in FORBIDDEN_PRODUCT_TERMS:
            if forbidden in schema_text:
                problems.append(f"{filename} contains product-specific term {forbidden}")
        for ref in local_refs(schema):
            target = (path.parent / unquote(ref.split("#", 1)[0])).resolve()
            if not target.is_relative_to(SCHEMA_ROOT.resolve()) or not target.is_file():
                problems.append(f"{filename} has unresolved local ref {ref}")

    install_ref_paths = [
        SCHEMA_ROOT / "install_ref" / "install_ref.v1.schema.json",
        SCHEMA_ROOT / "launcher" / "install_ref.v1.schema.json",
    ]
    for path in install_ref_paths:
        try:
            schema = load_schema(path)
        except (OSError, json.JSONDecodeError) as error:
            problems.append(f"invalid install-reference schema {path.relative_to(ROOT)}: {error}")
            continue
        properties = schema.get("properties", {})
        ownership = properties.get("ownership", {}).get("enum", [])
        if ownership != ["managed", "imported", "foreign_owned"]:
            problems.append(f"{path.relative_to(ROOT)} has incorrect ownership law")
        for field in (
            "setup_state_ref",
            "exact_product_version",
            "entrypoint",
            "capabilities",
            "lifecycle_status",
            "last_verification_identity",
            "state_revision",
        ):
            if field not in properties:
                problems.append(f"{path.relative_to(ROOT)} is missing {field}")

    if problems:
        for problem in problems:
            print(f"setup-handoff-contract-check: {problem}", file=sys.stderr)
        return 1
    print("setup-handoff-contract-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
