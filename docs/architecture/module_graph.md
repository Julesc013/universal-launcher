# Module Graph

```text
include/ulk
  -> runtime/launcher/kernel
  -> runtime/launcher/command
  -> runtime/launcher/product
  -> runtime/launcher/install_ref
  -> runtime/launcher/instance
  -> runtime/launcher/profile
  -> runtime/launcher/account
  -> runtime/launcher/artifact_set
  -> runtime/launcher/launch_plan
  -> runtime/launcher/diagnostics
  -> runtime/launcher/audit

runtime/binding
  -> product binding ABI

include/ulu
  -> runtime/client
  -> runtime/daemon
  -> runtime/platform
```

Product bindings provide product facts. The launcher owns orchestration and
launch planning, not setup mutation or product-specific rules.
