# Operation Outcomes

Every authority-bearing operation has two identities:

- `operation_id` identifies one durable logical operation;
- `attempt_id` identifies one dispatch attempt for that operation.

Both identifiers are mandatory, bounded, and transport-neutral. A retry may
retain the operation ID only when it is continuing the same durable operation;
it must use a new attempt ID.

## Terminal vocabulary

The operation contract exposes exactly these terminal outcomes:

```text
cancelled_before_dispatch
refused_before_effects
completed
cancellation_requested_but_completed
recovery_required
outcome_unknown
```

`cancelled_before_dispatch` and `refused_before_effects` are proof that effects
did not occur. `completed` preserves the provider's completed result.
`cancellation_requested_but_completed` means cancellation lost a race with a
provider that completed and returned an authoritative result.

`recovery_required` and `outcome_unknown` are fail-closed states:

```text
effects_may_have_occurred = true
recovery.required = true
recovery.inspect_command = non-empty canonical command ID
```

A transaction ID is included when the provider has one. The operation ID
remains the durable recovery reference when no provider transaction ID was
returned.

## Transport law

Cancellation is a request, not evidence that nothing happened. Before
dispatch, a transport may return `cancelled_before_dispatch`. After dispatch,
it must preserve a completed provider result or return `outcome_unknown` /
`recovery_required` with an inspection path. It may never replace a completed
provider response with ordinary cancellation.

Direct, process, and future daemon transports must project the same result for
the same provider facts. Transport failure may weaken knowledge to
`outcome_unknown`; it may not invent the stronger
`refused_before_effects` state.

The contract grants no Setup, network, credential, or product-execution
authority. Universal Setup remains the software-mutation owner, and product
bindings remain responsible for product-specific recovery commands.
