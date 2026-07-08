# Ecosystem Vision

Universal Launcher is the cross-product launcher spine. It should be proven by
FacMan and Factorio before it grows abstractly.

```text
Factorio proves the universal launcher through FacMan.
Dominium proves the universal setup.
FacMan ships as the first serious Factorio product binding.
```

Universal Launcher owns:

```text
products
install references
instances
profiles
account references
artifact sets
launch plans
command graph
dry-run / diagnostics / audit
daemon and client contracts
```

Universal Setup owns install mutation. FacMan owns Factorio discovery, launch
templates, modsets, saves, servers, Mod Portal rules, and account redaction.
The launcher may orchestrate those facts, but it must not redefine them.
