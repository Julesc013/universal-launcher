# Directory Structure

```text
universal-launcher/
  include/    public `ulk` C ABI headers
  runtime/    launcher kernel, command graph, daemon, client, platform adapters
  apps/       optional launcher frontends
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
