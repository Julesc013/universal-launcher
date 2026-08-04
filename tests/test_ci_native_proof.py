# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
WORKFLOW = ROOT / ".github" / "workflows" / "ci.yml"


class NativeCiProofTests(unittest.TestCase):
    def test_standalone_ci_builds_and_runs_native_tests(self) -> None:
        workflow = WORKFLOW.read_text(encoding="utf-8")
        required = [
            "actions/checkout@v6",
            "actions/setup-python@v6",
            "cmake -S . -B build/smoke -DCMAKE_BUILD_TYPE=Release",
            "cmake --build build/smoke --config Release",
            "ctest --test-dir build/smoke -C Release --output-on-failure",
            "python tools/strict_check.py",
        ]
        positions = [workflow.index(command) for command in required]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("runs-on: ${{ matrix.os }}", workflow)
        self.assertIn("fail-fast: false", workflow)
        for runner in ("ubuntu-latest", "windows-latest", "macos-latest"):
            self.assertIn(runner, workflow)
        self.assertIn("cmake -S . -B build/win32 -A Win32", workflow)
        self.assertIn("cppcheck --project=build/smoke/compile_commands.json", workflow)
        self.assertNotIn("actions/checkout@v4", workflow)
        self.assertNotIn("actions/setup-python@v5", workflow)


if __name__ == "__main__":
    unittest.main()
