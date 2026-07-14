# Setup Handoff

Universal Launcher may request setup-owned operations, but Universal Setup is
the mutation authority for install, verify, repair, uninstall, rollback,
installed-state, and setup audit.

## Handoff law

The launcher owns setup requests and references, not setup effects. A setup
apply request must carry the exact reviewed plan ID, plan digest, input identity
digest, provider identity, and provider revision. A setup result is accepted
only when all of those identities still match.

The product-neutral handoff contracts are:

- `ulk.setup_request.v1`
- `ulk.setup_plan_reference.v1`
- `ulk.setup_result.v1`
- `ulk.installed_state_reference.v1`
- `ulk.install_reference_refresh.v1`
- `ulk.setup_authority_status.v1`

Completed install results may create managed install references. Verify and
repair may refresh them. A repair result that changes the exact product version
is refused. Move refreshes a reference only after the new installed state has a
successful verification identity. Uninstall archives the reference and marks
dependent instances as unavailable instead of leaving broken paths.

Any accepted installed-state revision change makes launch plans bound to an
older revision stale. Frontends and product bindings must surface that state;
they must not silently rebuild or execute a plan.

A recovery-required result remains distinct even when an interrupted install
has not yet produced an install reference. The dependent-instance projection
is `managed_install_recovery_required`, and every bound launch plan is stale.
Existing instances and references are retained; the launcher neither guesses a
new path nor performs recovery itself.

No handoff contract exposes a generic mutation escape hatch. The launcher does
not extract archives, write installations, interpret product layouts, or
implement repair, move, uninstall, rollback, or recovery.

## Native projection

The additive `ulk` 1.3 ABI validates apply requests against reviewed plan
references, projects setup results into launcher-owned install-reference state,
and checks whether a launch plan remains fresh. The projection is allocation
free and performs no I/O. Returned string views borrow storage from the plan,
result, installed-state, or current-reference arguments and remain valid only
while that source storage remains valid.

Projection does not grant setup authority. It rejects foreign-owned and
imported references for managed lifecycle changes, rejects identity drift,
preserves the exact version across existing-install operations, requires an
active verified installed state before a move can refresh a reference, and
turns recovery or uninstall outcomes into structured status.
