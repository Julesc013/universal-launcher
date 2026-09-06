# ULK session and Last Run promotion 01

Date: 13 August 2026

State: `integrated_with_historical_synchronization`

## Exact candidate

```text
repository              Julesc013/universal-launcher
task branch             task/ulk-session-last-run-promotion-01
exact task base         85df03b292c09a004352b5e66cc6fc4d9fabae51
qualified session tree  b3b20261bf5e5cefa64c44e52d03cc5950295afc
session task parent     6a6e49336d66fb95ce6efce90d480d60f93e66ba
main before promotion   1cafe4054297cc11e02458b83d230db0cd064471
```

At qualification time, the exact `dev` history contained `main` and the
reviewed session task. No history was rewritten. This WorkUnit added promotion
truth and qualification evidence; it did not change journal behavior.

## Historical integration and synchronization

The original candidate identities and qualification below remain historical
evidence. Protected integration subsequently completed with normal merge
history:

```text
task evidence -> dev     e6de83ad1e1a2c646d31eb2ca68aa5cddb323b4a
dev -> main              09f0639ab6529fba2f2aa22e9bf68e5eebed0553
main -> dev              2e77e15c8bcdeb833a0a45aab3421886b72cc70c
1.9.1 -> main            5479939ca5cbc9ee0f901608a92012778b4752ae
1.9.1 main -> dev        5c2b6eb8ead53db863103a5190fa4fa130f64d42
August 20 shared tree    7728e4d415539a0f24e6f17aa7d22be00cc99d80
```

Those exact August 20 commits shared the recorded 1.9.1 source tree. This is
historical synchronization evidence.

## Branch observations on 6 September 2026

Read-only local observations of `origin/main` and `origin/dev` recorded:

```text
main revision           5479939ca5cbc9ee0f901608a92012778b4752ae
main tree               7728e4d415539a0f24e6f17aa7d22be00cc99d80
dev revision            0e8bcc38f5a55c80974c41da8d2eac10ac703593
dev tree                b28499daea1088504708691e794dbbbd59998f18
```

The observed `main` remains an ancestor of the observed `dev`. Their trees
are different: subsequent workspace-hygiene work changed `dev`. These dated
observations do not qualify this edited task candidate or establish equality
with its resulting tree, future merge commits, or subsequently moving refs.
FacMan's separate adoption WorkUnit consumes exact canonical pin
`5479939ca5cbc9ee0f901608a92012778b4752ae`.

Public ABI remains 1.9. The journal reads v1/v2 disk records and writes v2.
Generic process runtime, daemon/service execution, and stable `ulu` maturity
remain outside the implemented surface. There is no ULK tag or GitHub release;
the package remains experimental prerelease, unsigned, and unpublished.

## Qualified subset

The candidate is the product-neutral ABI 1.9 session journal already integrated
on `dev`: caller-rooted bounded persistence, session/operation/attempt and
opaque runnable identities, running and immutable terminal records, idempotent
replay, authoritative Last Run lookup, interruption recovery,
`outcome_unknown`, and `recovery_required`.

The package and C ABI remain experimental prerelease surfaces. The subset is a
qualified input to the separate FacMan adoption train; it is not a broad final
provider SPI and does not make any consumer pin current by itself.

## Promotion proof

The promotion requires fresh source reconstruction; ABI and symbol-additivity
checks; the native fault TCK; installed workspace, static, shared, and combined
consumers; relocated static/shared consumers; and interruption/corruption
recovery tests.

The local exact-tree qualification passed on Windows x64 with Visual Studio 18:

```text
py -3 tools/strict_check.py
  PASS

py -3 -m unittest discover -s tests -v
  19/19 PASS (17 product tests plus two promotion-record tests)

cmake -S . -B <external>/native -DULK_BUILD_TESTS=ON -DULK_BUILD_APPS=OFF
cmake --build <external>/native --config Release --parallel 4
ctest --test-dir <external>/native -C Release --output-on-failure --parallel 4
  15/15 PASS

py -3 tools/cmake_sdk_conformance.py --work-dir <external>/sdk-conformance \
  --config Release --platform x64 --phase full
  workspace/static/shared/combined PASS
  relocated static/shared PASS
  negative controls PASS
  ABI 1.9; composition, owned response, and session journal valid
```

Fresh shared-library builds of `main@1cafe405...` and the qualified candidate
were compared with the MSVC export-table dumper. All 34 existing exports are
preserved and the candidate has 40 exports: exactly six additive session
symbols. The frozen ABI 1.6 client, ABI layout, owned-response export, session
journal, idempotency, interruption, recovery, and corruption tests are included
in the green native/Python suites.

Migration review found no existing public ULK session-journal state to migrate.
ABI major 1 remains stable, ABI 1.9 is additive, and the frozen ABI 1.6 client
compatibility proof remains required.

## Closed scope

No process execution or supervision, daemon, dynamic provider, generic
reference-store expansion, Setup mutation, consumer repin/adoption, Factorio
terminology, stable public SPI claim, tag, release, signing, or publication is
authorized.

Normal repository flow remains:

```text
task promotion evidence -> dev
exact dev promotion -> main
main synchronization -> dev if required
separate FacMan exact-pin adoption
```
