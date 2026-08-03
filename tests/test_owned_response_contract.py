# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import owned_response_contract_check


class OwnedResponseContractTests(unittest.TestCase):
    def test_owned_response_contract_check(self) -> None:
        self.assertEqual(owned_response_contract_check.check(), [])


if __name__ == "__main__":
    unittest.main()
