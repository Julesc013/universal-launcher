# Boundary

Universal Launcher owns:

- products
- install references
- instances
- profiles
- account references
- artifact sets
- launch plans
- command graph
- dry-run and audit
- diagnostics
- daemon protocol
- frontend-neutral command clients

Universal Launcher does not own setup mutation. Universal Setup owns install,
verify, repair, uninstall, stage, commit, rollback, and installed-state
manifests.

Universal Launcher does not own Factorio semantics. Factorio discovery,
validation, launch templates, mods, modsets, saves, servers, Mod Portal rules,
and account redaction belong to `factorio-launcher`.

Public ABI namespaces:

- `ulk/` is the launcher kernel ABI for products, install references,
  instances, profiles, account references, artifact sets, launch plans,
  diagnostics, and audit.
- `ulu/` is the launcher utility/UI/platform ABI for command graph access,
  callbacks, UI models, platform interfaces, and host-facing reports.
