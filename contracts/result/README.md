# Result Contracts

Result contracts define generic success/result envelopes for command graph
surfaces. Payload-specific schemas remain under `contracts/schema/`.

Durable operation results use
`contracts/schema/command/operation_outcome.v1.schema.json`. They distinguish
pre-dispatch/no-effect outcomes from completed, recovery-required, and
post-dispatch unknown outcomes. A generic timeout or cancellation is never
proof that an authority-bearing provider produced no effects.
