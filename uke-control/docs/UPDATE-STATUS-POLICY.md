# Update status policy observations

5 November 2026. The read-only update status helper now reads both automatic
staging policies and the separately owned rollback policy. Missing or malformed
policy is reported as unknown and makes status incomplete. Reading status never
enables staging, accepts a kernel, changes a slot or changes preferences.

The text view uses two columns from60 terminal cells and three from120, wrapping
long releases within each column. Narrower terminals use one column. JSON keeps
the existing schema and adds `update_policy`. Automatic fallback is a policy
observation; it does not assert that matching recovery modules are installed.
Existing image, slot, approval and failed-candidate evidence remains separate.

The COPR constructor extends the complete SHA256-pinned installed
`uke-control-2.8.1-24.speech1.fc46` source from build11259934, preserving its
speech, client, rollback and frontend layers. Native tests run only in COPR.
Hardware fixes and actual alternating-slot staging belong to kernel/DT and the
boot updater respectively; this display change does not establish either.
