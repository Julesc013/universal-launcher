from __future__ import annotations

import re
import sys
import tomllib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
COMMAND_ROOT = ROOT / "contracts" / "command"
EFFECTS_POLICY = ROOT / "contracts" / "policy" / "effects.v1.toml"

EXPECTED_COMMANDS = {
    "command_graph.inspect",
    "install_refs.scan",
    "install_refs.import",
    "install_refs.list",
    "install_refs.inspect",
    "instance.create",
    "launch_plan.build",
    "diagnostics.report",
}

COMMAND_PATTERN = re.compile(r"^[a-z0-9_]+(\.[a-z0-9_]+)+$")


def main() -> int:
    problems: list[str] = []
    allowed_effects, effect_problems = load_effects_policy()
    problems.extend(effect_problems)

    seen: set[str] = set()
    for path in sorted(COMMAND_ROOT.glob("*.v1.toml")):
        contract, load_problems = load_toml(path)
        problems.extend(load_problems)
        if load_problems:
            continue
        command_id = contract.get("command_id")
        if isinstance(command_id, str):
            seen.add(command_id)
        problems.extend(validate_contract(path, contract, allowed_effects))

    for command_id in sorted(EXPECTED_COMMANDS - seen):
        problems.append(f"missing command contract {command_id}")
    for command_id in sorted(seen - EXPECTED_COMMANDS):
        problems.append(f"unexpected command contract {command_id}")

    if problems:
        for problem in problems:
            print(f"command-contract-check: {problem}", file=sys.stderr)
        return 1
    print(f"command-contract-check: ok ({len(seen)} commands)")
    return 0


def load_toml(path: Path) -> tuple[dict, list[str]]:
    try:
        with path.open("rb") as handle:
            data = tomllib.load(handle)
    except (OSError, tomllib.TOMLDecodeError) as exc:
        return {}, [f"{path.relative_to(ROOT)}: {exc}"]
    if not isinstance(data, dict):
        return {}, [f"{path.relative_to(ROOT)}: contract must be a TOML table"]
    return data, []


def load_effects_policy() -> tuple[set[str], list[str]]:
    policy, problems = load_toml(EFFECTS_POLICY)
    if problems:
        return set(), problems
    effects = policy.get("effects")
    if not isinstance(effects, dict) or not effects:
        return set(), [f"{EFFECTS_POLICY.relative_to(ROOT)}: effects table must be non-empty"]
    return set(effects), []


def validate_contract(path: Path, contract: dict, allowed_effects: set[str]) -> list[str]:
    problems: list[str] = []
    required = [
        "command_id",
        "owner",
        "effects",
        "request_schema",
        "response_schema",
        "result_schema",
        "refusal_schema",
        "dry_run_behavior",
        "refusal_codes",
        "cli_mapping_later",
    ]
    for key in required:
        if key not in contract:
            problems.append(f"{path.relative_to(ROOT)}: missing {key}")
    if problems:
        return problems

    command_id = contract["command_id"]
    if not isinstance(command_id, str) or not COMMAND_PATTERN.match(command_id):
        problems.append(f"{path.relative_to(ROOT)}: invalid command_id {command_id!r}")
    elif path.name != f"{command_id}.v1.toml":
        problems.append(f"{path.relative_to(ROOT)}: filename must match command_id")

    if contract["owner"] != "universal-launcher":
        problems.append(f"{path.relative_to(ROOT)}: owner must be universal-launcher")

    for key in ["request_schema", "response_schema", "result_schema", "refusal_schema"]:
        value = contract[key]
        if not isinstance(value, str) or not (ROOT / value).is_file():
            problems.append(f"{path.relative_to(ROOT)}: {key} does not exist: {value!r}")

    effects = contract["effects"]
    if not isinstance(effects, list) or not effects:
        problems.append(f"{path.relative_to(ROOT)}: effects must be a non-empty array")
    else:
        unknown = sorted(effect for effect in effects if effect not in allowed_effects)
        if unknown:
            problems.append(f"{path.relative_to(ROOT)}: unknown effects {unknown}")
        if "none" in effects and len(effects) != 1:
            problems.append(f"{path.relative_to(ROOT)}: none effect must be used alone")

    refusal_codes = contract["refusal_codes"]
    if not isinstance(refusal_codes, list) or not all(isinstance(code, str) for code in refusal_codes):
        problems.append(f"{path.relative_to(ROOT)}: refusal_codes must be an array of strings")

    for key in ["dry_run_behavior", "cli_mapping_later"]:
        if not isinstance(contract[key], str) or not contract[key].strip():
            problems.append(f"{path.relative_to(ROOT)}: {key} must be non-empty text")
    return problems


if __name__ == "__main__":
    raise SystemExit(main())
