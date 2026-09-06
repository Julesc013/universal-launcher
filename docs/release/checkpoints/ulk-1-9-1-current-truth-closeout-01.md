# ULK 1.9.1 current-truth closeout 01

Date: 26 August 2026; branch observations corrected 6 September 2026

State: `implementation_complete_pending_protected_integration`

## Boundary

This truth-only WorkUnit originally started from the historically synchronized
`dev@5c2b6eb8ead53db863103a5190fa4fa130f64d42`, tree
`7728e4d415539a0f24e6f17aa7d22be00cc99d80`. PR18 subsequently incorporated
`dev`; this correction preserves its existing head ancestry at
`484c6832a6c0ac1a4306d85987ee2b5a7c11bd90`.

It corrects stale roadmap, README, checkpoint, and machine-readable promotion
state. It changes no runtime source, public header, ABI, package layout, journal
format, command behavior, or provider authority.

## Dated observations on 6 September 2026

- ULK 1.9.1 is canonical on
  `main@5479939ca5cbc9ee0f901608a92012778b4752ae`.
- Local `origin/dev@0e8bcc38f5a55c80974c41da8d2eac10ac703593` contains the
  observed `main`, but its tree is `b28499daea1088504708691e794dbbbd59998f18`.
  The observed `main` tree remains `7728e4d415539a0f24e6f17aa7d22be00cc99d80`.
  The trees differ after workspace-hygiene integration.
- The August 20 synchronization at `dev@5c2b6eb8ead53db863103a5190fa4fa130f64d42`
  remains historical evidence. These branch observations make no tree-equality
  claim about this edited candidate or subsequent integration commits.
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
