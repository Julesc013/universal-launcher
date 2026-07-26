# Client

`ulk_client_v1` is the product-neutral frontend client seam. A binding supplies
one versioned adapter for `direct`, `process`, or `daemon` transport and ULK
performs common ABI, request, response, kind, and revision admission before
dispatch.

The adapter callback owns product and platform details. Direct adapters may
call a statically linked product binding; process adapters may supervise a CLI
or daemon process; daemon adapters may use a long-lived protocol. No transport
kind grants Setup, credential, network, or product execution authority.

Client initialization performs a shallow copy of the adapter and does not
allocate memory. The adapter object itself may go out of scope after
initialization, but `revision.data`, `user_data`, request views, and any
transport-owned response views remain borrowed. Their owners must keep them
valid for every call that can observe them. There is no client disposal call.

Frontend-neutral command clients for CLI, daemon, and C ABI transports.
