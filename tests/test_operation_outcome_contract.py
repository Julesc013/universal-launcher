# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import operation_outcome_contract_check


class OperationOutcomeContractTests(unittest.TestCase):
    def test_operation_outcome_contract_check(self) -> None:
        self.assertEqual(operation_outcome_contract_check.check(), [])


if __name__ == "__main__":
    unittest.main()
