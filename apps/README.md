# Apps

Frontend project/package shells. App roots are thin entrypoints over the
launcher command graph.

```text
apps/
  cli/
  tui/
  daemon/
  gui/
```

Reusable orchestration, client, daemon, binding, and platform behavior belongs
under `runtime/`.

Product-specific GUI matrices live in product repos. Universal Launcher may
host reference GUI shell policy, but it must not absorb WinForms, WinUI,
AppKit, SwiftUI, GTK, Qt, or product-specific UI behavior into the kernel.
