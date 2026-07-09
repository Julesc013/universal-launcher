# Language And Runtime Policy

Universal Launcher is a portable launcher kernel. Its public ABI is C, and its
runtime should stay compatible with conservative C/C++ implementation lanes
unless a specific platform adapter requires more.

## Kernel Policy

```text
include/ulk/        public launcher kernel C ABI
include/ulu/        public utility/UI/platform C ABI
runtime/launcher/   product-neutral orchestration kernel
runtime/client/     frontend-neutral command clients
runtime/daemon/     daemon and IPC host
runtime/platform/   low-level platform adapters
```

Public headers must not expose C++ classes, STL containers, exceptions,
templates, C# types, Swift types, Objective-C classes, or GUI toolkit objects.

## App Policy

CLI, TUI, and daemon app shells may live here as product-neutral reference
frontends. Product GUI matrices belong in product repos, not in Universal
Launcher.

## Compatibility

Portable helpers may use C++98-style implementation techniques behind C ABI
wrappers when justified. Do not name runtime folders after language standards;
use ownership names such as `portable_abi`, `command`, `client`, or
`platform`.
