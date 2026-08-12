# Session Journal

The experimental ABI 1.9 session journal is the minimum product-neutral
persistence boundary for durable runnable sessions and authoritative Last Run
lookup. A caller supplies the journal root and every fact. Universal Launcher
validates and commits those facts; it does not dispatch, monitor, cancel, or
recover a process.

Each record binds one session ID to one existing durable operation/attempt
identity and one opaque runnable reference. Records begin in `running` and may
make exactly one transition to `terminal`. Terminal outcomes reuse
`ulk.operation_outcome.v1`, including the distinctions between cancellation
before dispatch, completed-after-cancellation, recovery required, and unknown
outcome.

The store is deliberately bounded and inspectable:

- writes are idempotent and use a temporary file plus atomic replacement;
- terminal records are immutable;
- duplicate operation/attempt identities cannot name different sessions;
- a per-root lock serializes scans, writes, and retention;
- corrupt and future-version records fail closed;
- Last Run is the newest record for an exact opaque runnable reference;
- inspect, Last Run, and list use the public two-call caller-buffer law;
- a configurable bounded retention window defaults to 64 records.

This is reference persistence, not process services. There is no daemon,
dynamic provider system, product vocabulary, Setup authority, or live process
execution in this WorkUnit.
