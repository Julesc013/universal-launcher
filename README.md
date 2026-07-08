# Universal Launcher

Universal Launcher is the product-agnostic launcher kernel for products, install
references, instances, profiles, account references, artifact sets, launch plans, command graph,
dry-run, audit, diagnostics, daemon protocol, and frontend-neutral command
clients.

It does not install, repair, uninstall, or roll back products. Those operations
belong to Universal Setup. It also does not contain Factorio-specific discovery,
mods, saves, servers, launch templates, Mod Portal behavior, or account rules.
Those belong to product bindings such as FacMan in `factorio-launcher`.

## Ownership

```text
universal-setup     install / repair / uninstall / rollback authority
universal-launcher  cross-product orchestration and launch plans
factorio-launcher   FacMan product binding and app frontends
```

## Proof Role

```text
Factorio proves the universal launcher through FacMan.
Dominium proves the universal setup.
FacMan ships as the first serious Factorio product binding.
```

Universal Launcher should become real by supporting FacMan's Factorio
instances, profiles, artifact sets, launch plans, diagnostics, and command
graph. It should stay product-neutral while FacMan supplies Factorio-specific
facts.

Permanent rule:

```text
Universal setup mutates installed software state.
Universal launcher orchestrates runnable product state.
Product bindings interpret product-specific facts.
Frontends present commands and reports.
Contracts preserve compatibility.
Validators prevent regression.
```

## Durable Layout

```text
include/    public `ulk` kernel and `ulu` utility/UI/platform C ABI headers
runtime/    launcher kernel, binding host, command graph, daemon, client,
            platform adapters
apps/       optional launcher frontends; GUI providers live under `apps/gui/`
contracts/  ABI, command, schema, result, diagnostic, refusal, policy contracts
content/    universal launcher templates and policy
release/    package manifests and release profiles
docs/       human documentation
tests/      proof, fixtures, and golden outputs
tools/      validators and repo automation
cmake/      native build policy
archive/    retained planning/prototype material
```

Retired roots are forbidden:

```text
source/
src/
data/
schemas/
packaging/
setup/
factorio/
```

The app grammar is:

```text
apps/
  cli/
  tui/
  daemon/
  gui/
    win32/
    appkit/
    gtk/
    qt/
```

Use `artifact_set` for universal product-associated files. Factorio modsets are
one product binding's artifact-set semantics, not a universal launcher noun.

## Bootstrap Validation

```powershell
python tools\structure_policy_check.py
python -m unittest discover -s tests -v
cmake -S . -B build/native-smoke
cmake --build build/native-smoke
```

The current repository is a canonical bootstrap, not a production launcher
implementation yet.
