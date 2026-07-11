# Command Contracts

Universal Launcher command descriptors define the product-neutral command graph.
Product-specific commands belong in product bindings such as FacMan.

Each descriptor declares:

- `command_id`
- `owner`
- `effects`
- `request_schema`
- `response_schema`
- `result_schema`
- `refusal_schema`
- `dry_run_behavior`
- `refusal_codes`
- `cli_mapping_later`

The first registered-handler consumer covers `install_refs.scan`,
`install_refs.import`, `install_refs.inspect`, `instance.create`, and
`launch_plan.build`. Registration is static and in-process; it is not a dynamic
plugin ABI.

The registry accepts canonical command IDs only. Compatibility aliases belong
to frontend parsers and must be normalized before invocation.

`ulk_command_descriptor_v1` remains layout-compatible and now retains its
effects metadata. New registrations should use `ulk_command_descriptor_v2`,
which also retains request, response, result, and refusal schemas; dry-run
behavior; availability; owner; binding; and handler state for authoritative
introspection. Descriptor strings are copied into the context.
