# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import abi_contract_check


class AbiContractTests(unittest.TestCase):
    def test_public_symbols_and_struct_fields_match_snapshot(self) -> None:
        self.assertEqual([], abi_contract_check.check())


if __name__ == "__main__":
    unittest.main()
