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

## FACMAN-PROOF-01

- Use FacMan's real Factorio workflows to prove instances, profiles, artifact
  sets, launch plans, diagnostics, dry-run behavior, and frontend command graph
  contracts.
- Keep Factorio-specific facts in FacMan's product binding.

## SETUP-HANDOFF-01

- Hand install/verify/repair/uninstall/rollback to Universal Setup.
- Do not absorb setup mutation into the launcher.
