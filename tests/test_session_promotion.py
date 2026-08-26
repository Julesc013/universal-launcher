# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import copy
import unittest

from tools import session_promotion_check


class SessionPromotionTests(unittest.TestCase):
    def test_promotion_candidate_is_exact_and_non_authorizing(self) -> None:
        self.assertEqual(session_promotion_check.check(), [])

    def test_promotion_cannot_claim_stable_spi_or_consumer_adoption(self) -> None:
        with session_promotion_check.RECORD.open("rb") as handle:
            import tomllib

            record = tomllib.load(handle)
        changed = copy.deepcopy(record)
        changed["scope"]["stable_public_spi"] = True
        changed["scope"]["consumer_adoption"] = True
        problems = session_promotion_check.check_data(changed)
        self.assertIn("session promotion scope.stable_public_spi must remain false", problems)
        self.assertIn("session promotion scope.consumer_adoption must remain false", problems)

    def test_closeout_cannot_claim_publication_or_change_canonical_identity(self) -> None:
        with session_promotion_check.RECORD.open("rb") as handle:
            import tomllib

            record = tomllib.load(handle)
        changed = copy.deepcopy(record)
        changed["current"]["canonical_main_revision"] = "0" * 40
        changed["current"]["published"] = True
        problems = session_promotion_check.check_data(changed)
        self.assertIn(
            "session promotion current.canonical_main_revision must be "
            "'5479939ca5cbc9ee0f901608a92012778b4752ae'",
            problems,
        )
        self.assertIn("session promotion current.published must be False", problems)


if __name__ == "__main__":
    unittest.main()
