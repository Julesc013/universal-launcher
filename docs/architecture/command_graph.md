# Command Graph

The command graph is the shared model every frontend calls. CLI, TUI, daemon,
Product frontends are sibling views over the same command graph.

Each command should declare:

- command id
- request schema
- response schema
- dry-run behavior
- audit behavior
- redaction policy
- progress/cancellation behavior when needed

No frontend may introduce hidden behavior outside the command graph.

## Minimal v1 Kernel Commands

`ulk_command_execute_v1` currently exposes the product-neutral ULAUNCH-MIN-01
slice:

- `command_graph.inspect`
- `product.inspect`
- `install_refs.scan`
- `install_refs.import`
- `install_refs.list`
- `install_refs.inspect`
- `instance.create`
- `instance.list`
- `profiles.list`
- `account_refs.list`
- `artifact_sets.list`
- `launch_plan.build`
- `diagnostics.run`

These commands return `ulk.command_response.v1` JSON payloads and support
dry-run launch-plan preview. The built-in fallbacks remain generic; statically
linked product bindings register real handlers on a context and registered
handlers take precedence. The kernel does not
install, repair, uninstall, roll back, download, or mutate product installs.

Compatibility aliases are frontend-only:

- Frontends may map `instances.list` to `instance.list`.
- Frontends may map `diagnostics.report` to `diagnostics.run`.

The Universal Launcher registry accepts and dispatches only the canonical IDs.
`command_graph.inspect` is generated from built-in descriptors plus the
descriptors registered on that context. A registered descriptor overrides the
matching built-in metadata and handler status. Unregistering it restores the
built-in descriptor, or removes a registration-only command from introspection.

Introspection retains effects, request/response/result/refusal schemas, dry-run
behavior, availability, owner, binding, and whether the handler is built in or
registered. Its returned JSON storage belongs to the context and remains valid
until the next registry mutation, graph inspection, or context destruction.

The v1 descriptor layout is unchanged. The additive v2 descriptor and register
entrypoint provide the metadata required for this projection; this remains an
experimental ABI correctness floor rather than a stable third-party ABI claim.

Without a registered binding, `instance.create` is preview-only in the minimal kernel. Dry-run returns an
`ulk.instance_create_plan.v1` payload with `effects = ["workspace_write"]`.
Non-dry-run execution refuses with `product_binding_required` until a product
binding and workspace store own the concrete instance materialization.
