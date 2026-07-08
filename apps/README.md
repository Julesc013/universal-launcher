# Apps

Frontend project/package shells. App roots are thin entrypoints over the
launcher command graph.

```text
apps/
  cli/
  tui/
  daemon/
  gui/
    win32/
    appkit/
    gtk/
    qt/
```

Reusable orchestration, client, daemon, binding, and platform behavior belongs
under `runtime/`.
