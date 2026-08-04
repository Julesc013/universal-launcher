# Product composition contracts

ULK composition contracts describe product identity, exact version,
entrypoints, runnable artifact-set bindings, launch capabilities, and the exact
contract set used to interpret the composition. They are declarative and
frontend-neutral.

The closed capability vocabulary is `single_process`, `open_document`,
`multi_instance`, `profile_selection`, `artifact_sets`,
`session_supervision`, `background_service`, and `server`. Capabilities do not
classify a product and grant no process, session, persistence, or daemon
authority by themselves.

The neutral provider-local fixture uses `org.example.fixture`, one `main`
entrypoint at `bin/fixture`, the `core` artifact set, and the
`single_process` capability. Passing it qualifies only these composition
contracts as `fixture-qualified`.
