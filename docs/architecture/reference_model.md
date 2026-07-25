# Reference Model

Universal Launcher owns the product-neutral identities that compose a runnable
product state:

```text
Product
  -> InstallReference
  -> Instance
     -> optional Profile
     -> optional ArtifactSet
  -> LaunchPlan
```

The public C ABI exposes versioned value references and validation functions.
The JSON contracts under `contracts/schema/reference/` mirror those identities
without defining product-specific extensions.

Validation has two distinct outcomes:

- identity or composition disagreement is invalid and fails closed;
- a changed install-state or instance-binding revision leaves the graph valid
  but marks the launch plan stale.

The reference ABI does not persist records, mutate installations, interpret
product data, or execute processes. Universal Setup remains the only owner of
setup mutation. Product bindings remain responsible for their complete
instance specifications and workspace composition.
