# Architecture Overview

Universal Launcher is a deterministic orchestration kernel. It owns products,
install references, instances, profiles, accounts, launch plans, command graph,
dry-run, audit, diagnostics, daemon protocol, and frontend-neutral clients.

Product bindings answer product-specific questions without leaking those
semantics into the universal kernel.
