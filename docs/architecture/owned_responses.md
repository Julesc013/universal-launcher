# Owned Responses

The original `ulk_command_response_v1` ABI remains borrowed. Its string views
can point into a context, registered handler, or transport and must still be
copied before their documented owner invalidates them.

`ulk_owned_command_response_v1` is an additive lifetime boundary. The caller
validates and copies a borrowed response with
`ulk_command_response_copy_owned_v1`, reads the nested `response`, and releases
it with `ulk_owned_command_response_release_v1`.

## Validation and storage

Validation requires supported structure sizes, a known ULK status, and
well-formed null/size pairs for the payload, error message, and error detail.
Copying preserves their exact byte lengths in one contiguous allocation. Empty
views do not allocate and remain empty.

The combined copied bytes have a 1 MiB budget. Size addition is checked for
integer overflow before allocation or source dereference. A response exactly
at the budget is accepted; a larger or overflowing response is rejected.

The supplied allocator is copied into the owned object. A null allocator, or a
valid allocator with null callbacks, selects the library default. Allocation
failure leaves a cleared, releasable destination. Release is idempotent and does
not require the source context, handler, or transport to remain alive. A custom
allocator's callback code and `user` state must remain valid until release.
An owned response has one owner and must not be duplicated by copying its C
structure by value.

## Scope

Owned lifetime does not make a context concurrent. Existing registration,
execution, inspection, and destruction serialization rules are unchanged. It
also grants no process, network, setup, or product authority and does not alter
operation-outcome rules after dispatch.

The copy function reports response materialization only. Rejection or
allocation failure does not change the borrowed source and is not evidence
about whether a command produced effects.
