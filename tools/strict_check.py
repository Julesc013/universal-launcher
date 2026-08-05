# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import sys
from collections.abc import Callable
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools import (
    abi_contract_check,
    branch_policy_check,
    command_contract_check,
    language_runtime_policy_check,
    license_policy_check,
    operation_outcome_contract_check,
    owned_response_contract_check,
    product_composition_contract_check,
    reference_model_contract_check,
    sdk_package_check,
    setup_handoff_contract_check,
    structure_policy_check,
)


def main() -> int:
    checks: list[tuple[str, Callable[[], int]]] = [
        ("structure", structure_policy_check.main),
        ("abi-contract", abi_contract_check.main),
        ("branch-policy", branch_policy_check.main),
        ("language-runtime-policy", language_runtime_policy_check.main),
        ("license-policy", license_policy_check.main),
        ("command-contract", command_contract_check.main),
        ("operation-outcome-contract", operation_outcome_contract_check.main),
        ("owned-response-contract", owned_response_contract_check.main),
        ("product-composition-contract", product_composition_contract_check.main),
        ("setup-handoff-contract", setup_handoff_contract_check.main),
        ("reference-model-contract", reference_model_contract_check.main),
        ("sdk-package", sdk_package_check.main),
    ]
    failed: list[str] = []
    for name, check in checks:
        if check() != 0:
            failed.append(name)
    if failed:
        print(f"strict-check: failed checks: {', '.join(failed)}", file=sys.stderr)
        return 1
    print("strict-check: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
