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
