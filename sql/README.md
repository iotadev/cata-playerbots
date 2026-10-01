# Optional Playerbots database schema

`auth/001_factory_ownership.sql` is an additive schema for the configured auth
database. It creates only the module's ownership table. It does not create bot
accounts, ownership records or characters, and does not alter native auth tables.
It is not automatically applied by the core updater or at server startup.
Back up the database and review/apply it explicitly before enabling inspection.
Applying it is optional while the factory feature is disabled.

Each enrollment record represents an explicit operator decision to
dedicate one account to a character intent in one realm. The reader compares the
native account ID, exact account name and join-date epoch; only evidence version
1 is accepted. A foreign key removes orphaned module evidence when its native
account is deleted. No passwords, tokens, email addresses or client paths belong
in the table. It grants neither player control nor managed-session admission.

Enrollment and provisioning are console-only and default-off. Do not populate
evidence from matching name prefixes,
party invitations or an account-link configuration. Do not import real account
records into this repository. An administrator who can modify the database can
also modify this evidence; it is not a security boundary against administrators.

`Playerbots.Factory.InspectionEnabled = 1` enables the console-only diagnostic
`server playerbotdev managed inspect <account ID>`. Missing schema, missing or
stale evidence, failed queries and incompatible identities reject inspection.
Create/reuse results are drafts, not successful provisioning or admission.
Factory operations preflight the required optional column metadata through
information_schema before referencing the ownership table. This avoids the
core's fatal missing-table error for an absent optional schema. Apply the
documented schema before enabling mutations; do not alter/drop it while requests
are active. TrinityCore's global fatal SQL handling is not suppressed.
Reads are bounded operator diagnostics, not recurring world-update scans.

`Playerbots.Factory.Enabled = 1` also enables inspection and these console commands:

- `server playerbotdev managed enroll <account ID> <name> <race ID> <class ID> <gender 0|1>`
- `server playerbotdev managed provision <account ID>`
- `server playerbotdev managed factory-status <account ID>`

Create the ordinary account separately through native account administration.
`enroll` explicitly dedicates it to the normalized character intent, requires
an empty/ineligible-free account, and submits a plain INSERT. Existing records
are not replaced. One enrollment per account across realms is supported in this
first slice. Status confirms both the transaction result and the actual stored
record; a committed INSERT...SELECT with zero rows is not enrollment success.
There are no embedded credentials or password reset operations.

`provision` rechecks persisted ownership and current native identities. Missing
characters go through native validation/create/save/cache callbacks. One exact
existing character goes through a reserved accounting-only recovery context,
not another create attempt. Both require a confirmed realm-count repair before
status reports a ready GUID. Acknowledgment means submitted, not completed.
No automatic roster/config changes, admission or player-control grants occur.
Use the ready GUID with the existing explicit managed roster/admission workflow.

Up to 16 account attempt histories are retained in memory; reruns replace only a
terminal attempt for that account. Concurrent requests for a pending account
reject. Configuration disable prevents new mutations but pending enrollment
callbacks continue on the world thread. History does not survive restart; query
persistent evidence/identity and rerun provision rather than trusting old status.

The reader checks recorded other-realm counts in the native auth index, not the
contents of remote realm character databases. That index may be stale. Explicit
enrollment establishes dedication explicitly; this check cannot replace it.
The native creation path must still enforce name availability, limits and class
unlocks at submission time. Enrollment only writes evidence, and does not hold
the native provisioning reservation or promise cross-database atomicity. A client
login or character change during enrollment may make later provision reject;
creation/recovery rechecks eligibility and holds its own native reservation.

The schema and operator path passed the bundled disposable server check: absent
schema rejected without aborting, enrollment confirmed persistent evidence,
native creation completed, exact reuse repaired deliberately stale accounting
without a duplicate character, and conflicting intent rejected. The ready GUID
also completed explicit managed login/save/logout. This is native database/runtime
evidence, separate from pure policy tests. Ordinary client creation/accounting
and addon-managed lifecycle passed a separate bundled client check on 2026-10-01.
Linux runtime remains unverified. See ../PORTING.md.
