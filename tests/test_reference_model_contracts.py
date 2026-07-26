# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import reference_model_contract_check


class ReferenceModelContractTests(unittest.TestCase):
    def test_reference_model_contracts_are_closed_and_product_neutral(self) -> None:
        self.assertEqual([], reference_model_contract_check.check())


if __name__ == "__main__":
    unittest.main()
