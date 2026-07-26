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

Each graph contains exactly one record for each reference role. Duplicate
product, install, instance, profile, artifact-set, or plan identities therefore
cannot be admitted inside one graph. Callers that store collections must reject
duplicate record identities before selecting the singular records supplied to
this ABI.

Validation has two distinct outcomes:

- identity or composition disagreement is invalid and fails closed;
- a changed install-state or instance-binding revision leaves the graph valid
  but marks the launch plan stale.

Validation performs no allocation and retains no pointers. Every string view
and referenced record is borrowed only for the duration of the validation call.

The reference ABI does not persist records, mutate installations, interpret
product data, or execute processes. Universal Setup remains the only owner of
setup mutation. Product bindings remain responsible for their complete
instance specifications and workspace composition.
