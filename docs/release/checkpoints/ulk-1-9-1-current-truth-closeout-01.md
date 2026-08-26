# ULK 1.9.1 current-truth closeout 01

Date: 26 August 2026

State: `implementation_complete_pending_protected_integration`

## Boundary

This truth-only WorkUnit starts from exact synchronized
`dev@5c2b6eb8ead53db863103a5190fa4fa130f64d42`, tree
`7728e4d415539a0f24e6f17aa7d22be00cc99d80`.

It corrects stale roadmap, README, checkpoint, and machine-readable promotion
state. It changes no runtime source, public header, ABI, package layout, journal
format, command behavior, or provider authority.

## Current truth

- ULK 1.9.1 is canonical on
  `main@5479939ca5cbc9ee0f901608a92012778b4752ae`.
- `dev@5c2b6eb8ead53db863103a5190fa4fa130f64d42` contains `main` and has the
  same source tree.
- Public C ABI remains 1.9.
- The journal reads v1/v2 and writes v2.
- FacMan consumes exact canonical ULK pin
  `5479939ca5cbc9ee0f901608a92012778b4752ae`.
- Generic process runtime and daemon/service execution are not implemented.
- `ulu` remains experimental.
- No tag, signing, GitHub release, or publication exists.

## Authority

This closeout grants no execution, setup mutation, network, credential,
signing, publication, or product authority.
