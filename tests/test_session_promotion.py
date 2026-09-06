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

    def test_historical_sync_and_dated_dev_observation_are_distinct(self) -> None:
        with session_promotion_check.RECORD.open("rb") as handle:
            import tomllib

            record = tomllib.load(handle)
        historical = record["historical_synchronization"]
        current = record["current"]
        self.assertTrue(historical["main_and_dev_same_tree"])
        self.assertFalse(current["main_and_dev_same_tree"])
        self.assertEqual(historical["shared_tree"], current["canonical_main_tree"])
        self.assertNotEqual(current["canonical_main_tree"], current["observed_dev_tree"])
        self.assertEqual(session_promotion_check.check_data(record), [])

    def test_current_observation_rejects_historical_or_candidate_equivalence(self) -> None:
        with session_promotion_check.RECORD.open("rb") as handle:
            import tomllib

            record = tomllib.load(handle)
        mutations = {
            "main_and_dev_same_tree": True,
            "observed_dev_revision": record["historical_synchronization"]["synchronized_dev_revision"],
            "observed_dev_tree": record["historical_synchronization"]["shared_tree"],
            "candidate_tree_equivalence": "equal_to_observed_dev",
            "observation_kind": "live_remote_verification",
            "observed_on": "2026-08-20",
        }
        for field, value in mutations.items():
            with self.subTest(field=field):
                changed = copy.deepcopy(record)
                changed["current"][field] = value
                problems = session_promotion_check.check_data(changed)
                self.assertTrue(any(f"current.{field} must be" in problem for problem in problems), problems)
        changed = copy.deepcopy(record)
        changed["current"]["shared_tree"] = record["historical_synchronization"]["shared_tree"]
        self.assertIn(
            "session promotion current must enumerate exactly the dated observation fields",
            session_promotion_check.check_data(changed),
        )

    def test_historical_synchronization_identity_cannot_be_rewritten(self) -> None:
        with session_promotion_check.RECORD.open("rb") as handle:
            import tomllib

            record = tomllib.load(handle)
        record["historical_synchronization"]["synchronized_dev_revision"] = record["current"]["observed_dev_revision"]
        problems = session_promotion_check.check_data(record)
        self.assertTrue(any("historical_synchronization.synchronized_dev_revision" in problem for problem in problems), problems)


if __name__ == "__main__":
    unittest.main()
