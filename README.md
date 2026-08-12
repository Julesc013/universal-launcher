# Universal Launcher

Universal Launcher is the product-agnostic launcher kernel for products, install
references, instances, profiles, account references, artifact sets, launch plans, command graph,
dry-run, audit, diagnostics, daemon protocol, and frontend-neutral command
clients. It also owns product-neutral durable operation identity and terminal
outcome semantics shared by direct, process, and future daemon transports.
ABI 1.9 also exposes a bounded, caller-rooted session journal for durable
running/terminal records and Last Run lookup. It records consumer-supplied
facts and does not itself execute or monitor processes.

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
apps/       optional CLI, TUI, daemon, and reference app shells
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
```

Universal Launcher does not own product GUI matrices. Product repositories may
ship WinForms, WinUI, AppKit, SwiftUI, GTK, Qt, or other frontends over the
same command graph, daemon protocol, or C ABI. This repo keeps GUI ownership
to reference shell policy only.

Use `artifact_set` for universal product-associated files. Factorio modsets are
one product binding's artifact-set semantics, not a universal launcher noun.

## Branch and release train

Universal Launcher uses protected `main` and `dev` branches. `main` is stable
canonical provider source; `dev` is the continuously integrated next train and
must always contain `main`. Bounded `task/*` work starts from an exact `dev`
revision, targets `dev`, passes consumer canaries, and reaches `main` through a
reviewed promotion. Stable consumers retain exact pins reachable from `main`;
canary SHAs never rewrite their tracked locks.

See the [repository branch model](docs/governance/branch_model.md) and its
[machine-readable policy](release/index/branch_policy.v1.toml).

## Bootstrap Validation

```powershell
python tools\structure_policy_check.py
python -m unittest discover -s tests -v
cmake -S . -B build/native-smoke
cmake --build build/native-smoke
```

## CMake SDK

Release 1.8.0 can be installed and consumed without a sibling source checkout:

```powershell
cmake -S . -B build/sdk -DULK_BUILD_TESTS=OFF -DULK_BUILD_APPS=OFF
cmake --build build/sdk --config Release
cmake --install build/sdk --config Release --prefix out/ulk-sdk
```

An external consumer then uses
`find_package(UniversalLauncher 1.8.0 EXACT CONFIG REQUIRED)` and links one of
`UniversalLauncher::CoreStatic`,
`UniversalLauncher::CoreShared`, or the header-only
`UniversalLauncher::Headers` surface. See [the installed SDK guide](cmake/README-SDK.md).
The package version remains separate from C ABI 1.8 and fixture-qualified
contract maturity.

The current repository is an incremental production kernel. The authoritative
command graph, Setup handoff, frontend-neutral client/transport ABI,
product-neutral operation-outcome ABI, allocator-owned command-response ABI,
caller-selected owned-response budgets, and product-neutral reference
validation/composition ABI are implemented. Product descriptors, entrypoint
descriptors, launch capabilities, composition records, and contract-set
identity are defined by the additive ABI 1.8 contracts documented in
[`docs/architecture/product_composition.md`](docs/architecture/product_composition.md).
Reference persistence, daemon runtime, and platform process services remain
incomplete.

## License

Universal Launcher is licensed under the [MIT License](LICENSE). The canonical
machine-readable package identity is `release/license.v1.toml`. That license
choice does not imply signing, publication, or publisher authenticity; current
artifacts remain unsigned and unpublished.
