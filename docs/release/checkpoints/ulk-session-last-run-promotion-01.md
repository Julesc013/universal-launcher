# ULK session and Last Run promotion 01

Date: 13 August 2026

State: `qualified_task_candidate_pending_dev_integration`

## Exact candidate

```text
repository              Julesc013/universal-launcher
task branch             task/ulk-session-last-run-promotion-01
exact task base         85df03b292c09a004352b5e66cc6fc4d9fabae51
qualified session tree  b3b20261bf5e5cefa64c44e52d03cc5950295afc
session task parent     6a6e49336d66fb95ce6efce90d480d60f93e66ba
main before promotion   1cafe4054297cc11e02458b83d230db0cd064471
```

The exact current `dev` history contains `main` and the reviewed session task.
No history was rewritten. This WorkUnit adds promotion truth and qualification
evidence; it does not change journal behavior.

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
