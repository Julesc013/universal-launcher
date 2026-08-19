# SPDX-FileCopyrightText: 2026 Jules C
# SPDX-License-Identifier: MIT

from __future__ import annotations

import copy
import json
import tempfile
import unittest
from pathlib import Path

from tools import provider_package_manifest as provider_manifest

SOURCE_COMMIT = "1" * 40
SOURCE_TREE = "2" * 40


class ProviderPackageManifestTests(unittest.TestCase):
    def make_prefix(self, root: Path, linkage: str = "combined") -> Path:
        prefix = root / "prefix"
        files = {
            "include/ulk/ulk_api.h": "#define ULK_API_VERSION_MINOR 9\n",
            "include/ulu/ulu_abi.h": "#define ULU_ABI 1\n",
            "share/universal-launcher/contracts/abi/ulk_c_abi.v1.toml": (
                "abi_major = 1\nabi_minor = 9\n"
            ),
            "share/universal-launcher/contracts/schema/session/session_record.v1.schema.json": (
                '{"schema":"fixture"}\n'
            ),
            "share/licenses/universal-launcher/LICENSE": "MIT fixture\n",
            "share/licenses/universal-launcher/license.v1.toml": (
                'package_license_expression = "MIT"\n'
            ),
            "share/universal-launcher/README-SDK.md": "SDK fixture\n",
        }
        targets = ["UniversalLauncher::Headers"]
        if linkage in {"static", "combined"}:
            files["lib/ulk.lib"] = "static fixture\n"
            targets.append("UniversalLauncher::CoreStatic")
        if linkage in {"shared", "combined"}:
            files["bin/ulk.dll"] = "shared fixture\n"
            targets.append("UniversalLauncher::CoreShared")
        files["lib/cmake/UniversalLauncher/UniversalLauncherTargets.cmake"] = "".join(
            f"add_library({target} INTERFACE IMPORTED)\n" for target in targets
        )
        for relative, content in files.items():
            path = prefix / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(content, encoding="utf-8")
        return prefix

    def build(self, prefix: Path, linkage: str = "combined") -> dict[str, object]:
        return provider_manifest.build_manifest(
            prefix=prefix,
            source_repository="Julesc013/universal-launcher",
            source_commit=SOURCE_COMMIT,
            source_tree=SOURCE_TREE,
            source_ref="refs/heads/main",
            package_version="1.9.1",
            os_name="Windows",
            architecture="x64",
            linkage=linkage,
            configuration="Release",
            toolchain_id="MSVC",
            toolchain_version="19.44",
            compiler="C:/fixture/cl.exe",
        )

    def test_repository_package_truth_is_consistent(self) -> None:
        self.assertEqual(provider_manifest.check_repository(), [])

    def test_generation_is_byte_identical_and_verifies(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.make_prefix(Path(directory))
            first = self.build(prefix)
            path = provider_manifest.write_manifest(prefix, first)
            first_bytes = path.read_bytes()
            second = self.build(prefix)
            provider_manifest.write_manifest(prefix, second)
            self.assertEqual(path.read_bytes(), first_bytes)
            verified = provider_manifest.verify_manifest(
                prefix,
                expected_source_commit=SOURCE_COMMIT,
                expected_source_tree=SOURCE_TREE,
                expected_source_ref="refs/heads/main",
                expected_package_version="1.9.1",
                expected_c_abi="1.9",
                expected_journal_read_versions=[1, 2],
                expected_journal_write_version=2,
                expected_linkage="combined",
            )
            self.assertEqual(verified, first)

    def test_changed_or_missing_artifact_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.make_prefix(Path(directory))
            provider_manifest.write_manifest(prefix, self.build(prefix))
            header = prefix / "include/ulk/ulk_api.h"
            header.write_text("changed\n", encoding="utf-8")
            with self.assertRaisesRegex(provider_manifest.ManifestError, "artifact"):
                provider_manifest.verify_manifest(prefix)
            header.unlink()
            with self.assertRaisesRegex(provider_manifest.ManifestError, "artifact"):
                provider_manifest.verify_manifest(prefix)

    def test_identity_mismatches_are_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.make_prefix(Path(directory))
            provider_manifest.write_manifest(prefix, self.build(prefix))
            cases = {
                "expected_source_commit": "3" * 40,
                "expected_source_tree": "4" * 40,
                "expected_source_ref": "refs/heads/dev",
                "expected_package_version": "1.9.0",
                "expected_c_abi": "1.8",
                "expected_journal_read_versions": [2],
                "expected_journal_write_version": 1,
                "expected_linkage": "static",
            }
            for argument, value in cases.items():
                with self.subTest(argument=argument):
                    with self.assertRaises(provider_manifest.ManifestError):
                        provider_manifest.verify_manifest(prefix, **{argument: value})

    def test_changed_manifest_identity_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.make_prefix(Path(directory))
            path = provider_manifest.write_manifest(prefix, self.build(prefix))
            changed = copy.deepcopy(json.loads(path.read_text(encoding="utf-8")))
            changed["provider"]["package_version"] = "1.9.0"
            path.write_bytes(provider_manifest.canonical_json_bytes(changed))
            with self.assertRaisesRegex(provider_manifest.ManifestError, "1.9.1"):
                provider_manifest.verify_manifest(prefix)

    def test_mixed_linkage_target_inventory_is_refused(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            prefix = self.make_prefix(Path(directory), linkage="combined")
            with self.assertRaisesRegex(provider_manifest.ManifestError, "linkage"):
                self.build(prefix, linkage="static")


if __name__ == "__main__":
    unittest.main()
