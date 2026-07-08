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
