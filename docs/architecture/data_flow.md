# Data Flow

```text
product registry
  -> install references
  -> instances
  -> profiles
  -> account references
  -> artifact sets
  -> launch plan
  -> diagnostics/audit
```

Setup-owned operations are handed off to Universal Setup. Product-specific
questions are delegated to product bindings.
