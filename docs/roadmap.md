# Roadmap

Universal Launcher should be proven by FacMan and Factorio before it grows into
a broad abstract platform.

## ULAUNCH-CANON-01

- Keep the sibling root grammar enforced.
- Keep `ulk` and `ulu` public ABI families distinct.
- Keep launcher product-neutral and free of Factorio-specific discovery,
  modsets, saves, servers, Mod Portal behavior, and launch templates.

## ULAUNCH-MIN-01

- Product registry.
- Install reference model.
- Instance model.
- Profile model.
- Account reference model.
- Artifact set model.
- Launch plan model.
- Diagnostic report model.
- Command graph.
- JSON request/response model.

## ULAUNCH-REGISTRY-INTROSPECTION-02

- Generate `command_graph.inspect` from built-in and context-registered
  descriptors rather than a duplicated literal.
- Retain descriptor effects, schemas, dry-run behavior, availability, owner,
  binding, and handler status.
- Accept canonical command IDs in the registry; keep aliases in frontends.
- Preserve the v1 descriptor layout and add a versioned metadata descriptor.
- Prove graph/dispatch parity, unregister projection, metadata lifetime,
  duplicate and capacity refusal, allocator cleanup, and negative inputs.

## FACMAN-PROOF-01

- Use FacMan's real Factorio workflows to prove instances, profiles, artifact
  sets, launch plans, diagnostics, dry-run behavior, and frontend command graph
  contracts.
- Keep Factorio-specific facts in FacMan's product binding.

## SETUP-HANDOFF-01

- Hand install/verify/repair/uninstall/rollback to Universal Setup.
- Do not absorb setup mutation into the launcher.
