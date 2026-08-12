# ULK session/Last Run subset 01

Date: 12 August 2026

State: `implementation_green_pending_review`

## Exact source and parentage

```text
repository  Julesc013/universal-launcher
branch      task/ulk-session-last-run-subset-01
start/base  7d4fd8e25a8d529279c4ad18d983e9cd51839eb7
code head   ddbfe026bb51aad3ad40f348a69a03ec365957d3
parent      7d4fd8e25a8d529279c4ad18d983e9cd51839eb7
tree        clean after code commit; this checkpoint is the only closeout delta
```

The branch is based on the exact current remote `dev` observed at task start.
No provider branch or protected ref was changed.

## Result

ULK now exposes an experimental C ABI 1.9 product-neutral session subset:
caller-owned journal root, session/operation/attempt identities, opaque runnable
reference, running and terminal records, bounded list and Last Run lookup, and
the existing generic terminal outcome vocabulary. Writes are atomic,
idempotent, locked per journal, bounded by retention, CRC-protected, and reject
terminal mutation, identity conflicts, future schemas, corruption, path
escapes, and unsafe Windows reparse traversal.

Interruption leaves a running record available for caller-owned recovery.
Temporary interrupted writes are ignored. `outcome_unknown` and
`recovery_required` retain their recovery references and are never collapsed
into cancellation or generic failure.

Changed areas are the public C headers and ABI manifest, launcher kernel,
session schemas/validator, contract maturity record, native fault suite,
installed SDK consumers, package validators, and architecture/SDK docs.

## Authority before and after

Before: ULK owned generic operation outcomes but had no durable reference
session journal. After: it owns a fixture-qualified durable generic journal and
Last Run lookup. It still does not dispatch or supervise processes through this
API, choose a storage root, name Factorio concepts, load dynamic providers,
run a daemon, mutate installed software, or grant product execution, signing,
publication, or release authority. Maturity is
`fixture_qualified_experimental`; stable consumer adoption is not claimed.

## Validation

Host: Windows x64; MSVC Visual Studio generator; CPython 3.11.

```text
py -3.11 tools/strict_check.py
PASS: structure, ABI/symbol, branch, language, licence, command, operation,
session, owned-response, composition, setup-handoff, reference, SDK package

py -3.11 -m unittest discover -s tests -v
PASS: 17/17

ctest --test-dir build/session-journal -C Debug --output-on-failure
PASS: 15/15
  native session journal and ABI tests
  installed workspace consumer
  installed static consumer
  installed shared consumer
  installed combined consumer
```

The native session suite covers exact duplicate writes, changed-input
conflicts, terminal immutability, bounded buffer negotiation, retention,
Unicode roots, CRC corruption, valid-CRC future schemas, interrupted temporary
records, recovery required, and outcome unknown. Static and shared builds use
warning-as-error policy. Installed consumers write and query a real temporary
journal rather than checking headers alone.

No hosted run existed when this checkpoint was authored; hosted run IDs and
URLs must be appended to the draft PR after the synchronized head runs. There
were no skips. Package evidence is the four passing installed CMake SDK
consumer modes; no release artifact, signing, or publication was created.

## Known limitations and unchanged systems

No reference process service or full runtime session supervisor was added.
The caller remains responsible for root custody, clock values, process facts,
and recovery actions. Stable `main`/`dev`, FacMan provider locks, Universal
Setup, Factorio installations and archives, tags, releases, credentials, and
all real processes are unchanged.

## Next dependency-ordered WorkUnits

1. Review and integrate the FacMan prerequisite stack #133 → #134 → #135.
2. Review the FacMan CLI/scope/presentation foundation against its exact base.
3. Review and promote this bounded ULK subset, then reconcile FacMan's stable
   provider pin through its normal reviewed provider-adoption process.
4. Build an engineering-only atomic WinForms/AppKit/GTK canary using the
   promoted ULK provider; do not switch a subset of frontends.
5. Qualify the fixture existing-install journey and unsigned Windows WinForms
   Technical Preview package on clean supported hosts.
6. Resolve the 2.0.77 versus 2.1.14 route evidence decision and separately
   qualify one real Play route; keep managed installation deferred.
