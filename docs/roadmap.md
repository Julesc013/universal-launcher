# Roadmap

Universal Launcher should be proven by FacMan and Factorio before it grows into
a broad abstract platform.

## ULAUNCH-CANON-01

- Keep the sibling root grammar enforced.
- Keep `ulk` and `ulu` public ABI families distinct.
- Keep launcher product-neutral and free of Factorio-specific discovery,
  modsets, saves, servers, Mod Portal behavior, and launch templates.

## ULAUNCH-MIN-01

- Product registry and reference validation are implemented; durable registry
  persistence remains.
- Install reference projection and validation are implemented; persistence
  remains.
- Instance reference validation is implemented; product bindings retain full
  instance specifications.
- Profile reference validation is implemented.
- Account reference model.
- Artifact-set reference validation is implemented.
- Launch-plan binding and staleness projection are implemented; executable
  plan construction remains product-binding work.
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

## M1-WU8-SETUP-HANDOFF

- Bind immutable reviewed setup plan identities to apply requests.
- Accept only identity-matching setup results from the selected provider.
- Create or refresh managed install references after verified completion.
- Preserve product version across repair.
- Refresh move references only after the destination verifies.
- Archive uninstalled references and surface structured dependent-instance
  status.
- Mark launch plans stale whenever their installed-state revision changes.
- Keep every setup filesystem effect outside Universal Launcher.
