# Ownership

Universal Launcher owns runnable product orchestration: products, install
references, instances, profiles, account references, artifact sets, launch
plans, command graph, daemon protocol, diagnostics, and audit.

It does not own setup mutation and does not own Factorio semantics.

Managed install references may point to Universal Setup installed-state
records. Imported and foreign-owned references remain explicitly distinct and
do not gain setup ownership merely by being registered with the launcher.
