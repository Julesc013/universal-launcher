# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest

from tools import structure_policy_check


class StructurePolicyTests(unittest.TestCase):
    def test_structure_policy_check(self) -> None:
        self.assertEqual(structure_policy_check.main(), 0)


if __name__ == "__main__":
    unittest.main()
