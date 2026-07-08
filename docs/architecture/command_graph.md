# Command Graph

The command graph is the stable model every frontend calls. CLI, TUI, daemon,
WinForms, AppKit, GTK, and Qt are sibling views over the same command graph.

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
- `install_refs.list`
- `instances.list`
- `profiles.list`
- `account_refs.list`
- `artifact_sets.list`
- `launch_plan.build`
- `diagnostics.report`

These commands return `ulk.command_response.v1` JSON payloads and support
dry-run launch-plan preview. The built-in registry is intentionally empty and
generic; product bindings supply product-specific facts. The kernel does not
install, repair, uninstall, roll back, download, or mutate product installs.
