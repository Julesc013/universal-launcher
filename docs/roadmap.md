# Roadmap

Universal Launcher should be proven by FacMan and Factorio before it grows into
a broad abstract platform.

Every new WorkUnit starts from an exact current `dev`, integrates through
`task/* -> dev`, passes exact-SHA consumer canaries, and is promoted separately
to canonical `main`. Consumer adoption remains a later exact-pin pull request.

## ULK-SESSION-LAST-RUN-PROMOTION-01 (complete)

- Qualified task evidence entered `dev` through merge commit
  `e6de83ad1e1a2c646d31eb2ca68aa5cddb323b4a`.
- Reviewed `dev` reached canonical `main` through merge commit
  `09f0639ab6529fba2f2aa22e9bf68e5eebed0553`, then returned to `dev`
  through `2e77e15c8bcdeb833a0a45aab3421886b72cc70c`.
- The 1.9.1 Last Run ordering/package truth reached canonical `main` through
  `5479939ca5cbc9ee0f901608a92012778b4752ae`; the August 20 synchronization
  at `dev@5c2b6eb8ead53db863103a5190fa4fa130f64d42` shared its tree.
- The 6 September local observation of
  `dev@0e8bcc38f5a55c80974c41da8d2eac10ac703593` still contains that `main`,
  but its tree differs after workspace-hygiene work. Historical synchronization
  does not establish tree equality for this edited candidate or future refs.
- FacMan separately adopted exact canonical pin
  `5479939ca5cbc9ee0f901608a92012778b4752ae`.
- ABI remains 1.9. The journal reads disk formats v1/v2 and writes v2.
- No process execution, daemon/service runtime, stable public SPI, tag,
  signing, or publication was introduced.

## ULK-SESSION-LAST-RUN-SUBSET-01 (integrated and promoted)

- Exact base: `dev@7d4fd8e25a8d529279c4ad18d983e9cd51839eb7`.
- Add product-neutral session/operation/attempt identity, running and terminal
  records, bounded caller-rooted persistence, inspect/list, and authoritative
  Last Run lookup.
- Preserve `outcome_unknown` and `recovery_required`, fail closed on corrupt or
  future records, and prove idempotent/interruption-safe writes through the
  native ABI and installed SDK consumer.
- Add no process execution, daemon, dynamic provider, Setup mutation, product
  terminology, consumer repin, signing, or publication authority.

## ULK-CMAKE-SDK-PACKAGE-01 (canonical on main)

- Exact base: `dev@719a3ec240831547071d69098e1fe8c76f327fb7`.
- Package only the accepted ABI 1.8 and fixture-qualified contracts as
  `UniversalLauncher::Headers`, `UniversalLauncher::CoreStatic`, and
  `UniversalLauncher::CoreShared`.
- Prove source, installed static, installed shared, and relocated installed
  modes with one neutral external consumer and identical normalized results.
- Keep FacMan's stable ULK pin unchanged and open no process, persistence,
  session, setup, consumer-adoption, signing, publication, or product authority.
- Leave `USK-CMAKE-SDK-PACKAGE-01` inactive until this package is reviewed.

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

## ULK-OPERATION-OUTCOME-CONTRACT-01

- Require durable operation and attempt identity.
- Distinguish cancellation before dispatch from cancellation requested after
  dispatch.
- Preserve completed provider responses when cancellation loses a race.
- Treat timeout, transport loss, and unproven post-dispatch cancellation as
  `outcome_unknown` with effects and recovery inspection declared.
- Keep direct, process, and future daemon transports semantically identical.
- Grant no Setup, network, credential, or product-execution authority.

## ULK-OWNED-RESPONSE-ABI-01

- Validate command-response structure and borrowed string views explicitly.
- Copy payload and error views into one allocator-owned, budgeted block.
- Reject overflow and over-budget responses before allocation or copying.
- Preserve the 1 MiB convenience boundary while allowing an explicit,
  platform-representable caller-selected aggregate limit.
- Make release idempotent and independent of source context lifetime.
- Preserve the existing borrowed command and client ABIs.

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
