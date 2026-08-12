# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import session_contract_check


class SessionContractTests(unittest.TestCase):
    def test_session_contract_check(self) -> None:
        self.assertEqual(session_contract_check.check(), [])


if __name__ == "__main__":
    unittest.main()
