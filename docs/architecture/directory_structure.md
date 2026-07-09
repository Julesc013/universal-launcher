# Directory Structure

```text
universal-launcher/
  include/    public `ulk` kernel and `ulu` utility/UI/platform C ABI headers
  runtime/    launcher kernel, binding host, command graph, daemon, client,
              platform adapters
  apps/       optional CLI, TUI, daemon, and reference app shells
  contracts/  ABI, command, schema, result, diagnostic, refusal, policy
  content/    universal launcher templates and policy
  release/    package manifests and release profiles
  docs/       human documentation
  tests/      proof, fixtures, and golden outputs
  tools/      validators and repo automation
  cmake/      native build policy
  archive/    retained planning/prototype material
```

`source/` and `src/` are retired. Do not create implementation bucket
directories. Implementation belongs under `runtime/` or app entrypoint roots.

Universal Launcher must not contain setup mutation or product-specific behavior.
It owns orchestration, command graph, profiles, instances, install references,
accounts, launch plans, audit, diagnostics, daemon protocol, and client
contracts.

## App Shells

```text
apps/
  cli/
  tui/
  daemon/
  gui/
```

Universal Launcher keeps GUI ownership to product-neutral reference policy.
Concrete product GUI stacks live in product repos and call the same command
graph, daemon protocol, JSON transport, or C ABI.

## Launcher Runtime Modules

```text
runtime/launcher/
  kernel/
  command/
  product/
  discovery/
  install_ref/
  instance/
  profile/
  account/
  artifact_set/
  launch_plan/
  compatibility/
  diagnostics/
  export/
  audit/
```

`artifact_set` is the universal noun. Product bindings may interpret an
artifact set as mods, packs, plugins, DLC, tools, or another product-specific
file set.
