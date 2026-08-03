# Client

`ulk_client_v1` is the product-neutral frontend client seam. A binding supplies
one versioned adapter for `direct`, `process`, or `daemon` transport and ULK
performs common ABI, request, kind, and revision admission before dispatch.
Returned command responses can be admitted with
`ulk_command_response_validate_v1` before a caller interprets their views.

The adapter callback owns product and platform details. Direct adapters may
call a statically linked product binding; process adapters may supervise a CLI
or daemon process; daemon adapters may use a long-lived protocol. No transport
kind grants Setup, credential, network, or product execution authority.

Client initialization performs a shallow copy of the adapter and does not
allocate memory. The adapter object itself may go out of scope after
initialization, but `revision.data`, `user_data`, request views, and any
transport-owned response views remain borrowed. Their owners must keep them
valid for every call that can observe them. There is no client disposal call.

Callers that need a response beyond its producer's borrowed lifetime can use
`ulk_command_response_copy_owned_v1`. It validates the response and copies its
payload, error message, and error detail into one budgeted allocator-owned
block. `ulk_owned_command_response_release_v1` is idempotent and does not
depend on the producer remaining alive. Existing borrowed client and command
response layouts are unchanged.

The convenience copy retains a 1 MiB aggregate limit. The additive
`ulk_command_response_copy_owned_with_options_v1` entry point accepts a
caller-selected exact limit with no hidden 1 MiB ceiling. Null options select
the convenience defaults; a zero-initialized options limit selects the same
1 MiB default. Any nonzero limit is exact. All aggregate overflow,
selected-limit, and platform-representability checks occur before allocation
or source-byte copying.

Frontend-neutral command clients for CLI, daemon, and C ABI transports.

The additive operation-outcome ABI supplies durable operation/attempt identity
and fail-closed terminal semantics shared by every transport. Legacy
`ulk_client_v1` request and response layouts are unchanged. A cancellation or
timeout after dispatch must preserve a completed provider response or surface
`outcome_unknown` / `recovery_required`; it cannot claim that no effects
occurred.
