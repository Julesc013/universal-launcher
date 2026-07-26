# Launch Plan Model

Launch plans are dry-run-first descriptions of executable path, arguments,
environment, working directory, data roots, diagnostics, and refusal reasons.
Execution is a separate opt-in step.

`ulk_launch_plan_ref_v2` binds a plan to product, install, instance, optional
profile and artifact-set identities plus the install-state and instance-binding
revisions used to construct it. `ulk_reference_graph_validate_v1` treats
identity disagreement as an invalid graph and revision drift as a valid but
stale launch plan.

The reference model contains no process authority and does not interpret
product-specific arguments, environment, data roots, or artifacts.
