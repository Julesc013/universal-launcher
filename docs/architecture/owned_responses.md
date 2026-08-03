# Owned Responses

The original `ulk_command_response_v1` ABI remains borrowed. Its string views
can point into a context, registered handler, or transport and must still be
copied before their documented owner invalidates them.

`ulk_owned_command_response_v1` is an additive lifetime boundary. The caller
validates and copies a borrowed response with
`ulk_command_response_copy_owned_v1`, reads the nested `response`, and releases
it with `ulk_owned_command_response_release_v1`.

The convenience copy function retains its 1 MiB aggregate limit. Callers that
need a different explicit boundary use
`ulk_command_response_copy_owned_with_options_v1` and a versioned
`ulk_owned_command_response_options_v1`.

## Validation and storage

Validation requires supported structure sizes, a known ULK status, and
well-formed null/size pairs for the payload, error message, and error detail.
Copying preserves their exact byte lengths in one contiguous allocation. Empty
views do not allocate and remain empty.

The three copied view lengths are added with checked arithmetic before
allocation or source-byte dereference. The resulting aggregate must not exceed
the selected limit and must fit the platform's addressable `size_t` range. A
response exactly at the selected boundary is accepted; a larger,
unrepresentable, or overflowing response is rejected before allocation.

For the options entry point, a null options pointer selects the same default
allocator and 1 MiB limit as the convenience function. A non-null options
object must set `struct_size`. Its `maximum_total_bytes` is exact: zero permits
only a response with zero copied bytes. There is no hidden 1 MiB ceiling on
this path; for example, a caller may explicitly permit 16 MiB when the platform
can represent it.

The supplied allocator is copied into the owned object. A null allocator, or a
valid allocator with null callbacks, selects the library default. The options
and allocator structures themselves need remain valid only for the copy call.
Allocation failure leaves a cleared, releasable destination. Release is
idempotent and does not require the source context, handler, or transport to
remain alive. A custom allocator's callback code and `user` state must remain
valid until release. An owned response has one owner and must not be duplicated
by copying its C structure by value.

## Scope

Owned lifetime does not make a context concurrent. Existing registration,
execution, inspection, and destruction serialization rules are unchanged. It
also grants no process, network, setup, or product authority and does not alter
operation-outcome rules after dispatch.

The copy function reports response materialization only. Rejection or
allocation failure does not change the borrowed source and is not evidence
about whether a command produced effects.

## Compatibility proof

`contracts/abi/ulk_c_abi.v1.toml` binds the reviewed ABI 1.6 base, the exact
ABI 1.7 public symbol set, and the field order of borrowed and owned C
structures. Native proof compares the current borrowed layouts with frozen
ABI 1.6 layouts, links the owned-response surface through the shared library,
and runs a client compiled only from frozen ABI 1.6 declarations against the
current library. The caller-selected budget addition does not change the ABI
minor because it is part of the pre-acceptance ABI 1.7 review train.
