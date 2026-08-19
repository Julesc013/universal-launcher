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
- Last Run is the newest durably admitted session for an exact opaque runnable
  reference, even when caller timestamps are equal;
- inspect, Last Run, and list use the public two-call caller-buffer law;
- a configurable bounded retention window defaults to 64 records.

Disk record v2 adds an internal, lock-serialized commit order. It does not
change the public ABI or JSON schema. Readers accept legacy v1 disk records;
the first later v2 admission sorts after the legacy set, and subsequent v2
records sort by durable admission order. Terminal replacement preserves the
session's original order. This prevents clock precision, clock skew, random
session IDs, or directory enumeration order from selecting an older record as
Last Run.

This is reference persistence, not process services. There is no daemon,
dynamic provider system, product vocabulary, Setup authority, or live process
execution in this WorkUnit.
