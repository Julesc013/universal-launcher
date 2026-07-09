# Compatibility Tiers

Universal Launcher compatibility is capability-tiered.

```text
U0 - Minimal manifest and command-record subset
U1 - Hosted portable command graph and local file state
U2 - Native process, path, IPC, and daemon integration
U3 - Modern diagnostics, packaging, signing, and scheduled CI lanes
U4 - Product GUI frontends outside the universal kernel
```

The same command contracts can travel across tiers, but feature availability
depends on the platform and product binding.
