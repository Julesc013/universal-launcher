# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import copy
import tomllib
import unittest

from tools import sdk_package_check


class SdkPackageTests(unittest.TestCase):
    def test_canonical_sdk_package_record_and_build_are_valid(self) -> None:
        self.assertEqual(sdk_package_check.check(), [])

    def test_contract_maturity_cannot_advance_during_packaging(self) -> None:
        with sdk_package_check.RECORD.open("rb") as handle:
            record = tomllib.load(handle)
        invalid = copy.deepcopy(record)
        invalid["contract_maturity"] = "stable"
        self.assertIn(
            "SDK WorkUnit contract_maturity must be 'fixture-qualified'",
            sdk_package_check.check_data(invalid),
        )

    def test_packaging_cannot_open_product_execution(self) -> None:
        with sdk_package_check.RECORD.open("rb") as handle:
            record = tomllib.load(handle)
        invalid = copy.deepcopy(record)
        invalid["scope"]["product_execution"] = True
        self.assertIn(
            "SDK WorkUnit scope.product_execution must remain false",
            sdk_package_check.check_data(invalid),
        )


if __name__ == "__main__":
    unittest.main()
