# Instance Model

Instances are runnable product workspaces associated with an install reference,
profile choices, artifact sets, account references, and launch-plan options.

`ulk_instance_ref_v2` records only cross-product identity and binding:
product, install, optional profile and artifact-set references, and a binding
revision. Product-specific instance specifications, filesystem layout, and
content remain in the product binding.

The reference-graph validator rejects cross-product or cross-install
composition before launch-plan staleness is considered.
