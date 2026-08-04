# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import product_composition_contract_check


class ProductCompositionContractTests(unittest.TestCase):
    def test_contracts_are_closed_neutral_and_fixture_qualified(self) -> None:
        self.assertEqual([], product_composition_contract_check.check())


if __name__ == "__main__":
    unittest.main()
