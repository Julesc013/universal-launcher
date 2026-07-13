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

No handoff contract exposes a generic mutation escape hatch. The launcher does
not extract archives, write installations, interpret product layouts, or
implement repair, move, uninstall, rollback, or recovery.
