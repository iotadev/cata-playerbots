# Cataclysm Playerbots

An experimental port of [AzerothCore mod-playerbots](https://github.com/mod-playerbots/mod-playerbots)
to TrinityCore's Cataclysm 4.3.4 branch. The aim is the familiar Playerbots
feature set on Cata, not a new bot architecture or a standalone server package.

This repository contains the module. It is **not installable against stock
TrinityCore**. Use the matching source snapshot of the
[iotadev TrinityCore fork](https://github.com/iotadev/TrinityCore); the core
README records the module revision tested with that snapshot. An arbitrary
core/module pair is not assumed compatible.

## Current state

The playable slice is a manually configured, low-level companion party:

- Four bot slots can accept a human's party invitation, follow, and enter an
  instance with the group.
- Starter Warrior and Mage combat, Priest healing (including self-healing), and
  basic party buffs work in the local Cata test realm.
- Server-origin login, transfer, logout, and shutdown use the core's normal
  player/session paths.

The 2026-09-29 disposable mixed-party playtest reached Ragefire Chasm. The Mage
opened on a hostile target and logged 13 accepted offensive casts; the Priest
logged six healing casts and one Fortitude cast. Warrior combat routing was
observed, but this run did not establish Warrior threat or an accepted Warrior
ability. It also did **not** establish a complete dungeon clear, dependable
death recovery, or full class/spec rotations.
Bot accounts are still supplied manually. An optional console-only factory can
create or reuse an explicitly dedicated character through native Cata creation;
it does not automatically populate or admit bots. Autonomous population,
questing, and world progression have not been ported.

An initial managed existing-character roster is also present. When explicitly
enabled, a console operator can list configured account/character pairs and
request native login or logout with `server playerbotdev managed list`,
`start <character GUID>`, and `stop <character GUID>`. It does not create
characters, connect them automatically, or grant a human control based on
account ownership. Explicit account links and existing party authorization
provide the separate player-control boundary. The addon-managed roster and
connect/disconnect/reconnect path passed a bundled ordinary linked-player client
check on 2026-10-01, including native completion and clean shutdown.
`managed list` reports the latest attempt as loading, online, exit pending,
stopped, login failed, disconnected or shutdown. A completed exit means the
native logout/save path returned and the session closed; it does not certify
that asynchronous database writes have committed. Receipts are kept in memory
for the configured identity and are replaced on its next accepted admission.

The player lifecycle service is now implemented behind
`Playerbots.Managed.AllowPlayerControl = 0`. When enabled, ordinary players
need a directional trusted account link in `Playerbots.Managed.AccountLinks`
and must share the bot's faction. Stopping a grouped bot also requires its
current party control; GMs retain the explicit override. Links do not adopt a
party controller or change invitation eligibility. Malformed link settings
disable player lifecycle access. The service can list authorized configured
offline bots and return receipts for start/stop requests. The default-off
MultiBot transport now calls this service. This is compile/test coverage, not a live
ordinary-player permission claim.

An experimental normal-whisper path accepts `follow`, `stay`, `attack`, and
`stop` from the bot's authorized party controller; `list` shows that player's
currently online controllable bots. The latest client test confirmed `list`
and the basic movement/control behavior. One `attack` attempt after `stop` was
rejected because the human's selected target was Botmage, not a hostile unit;
the command's target-selection UX needs further work. This is not the full
upstream command set or the MultiBot addon bridge, and it cannot connect an
offline bot.

Use `/w Botmage list` (substitute an admitted bot's name) to view your active
controllable roster. For `attack`, select a hostile unit before whispering the
command and have the bot following you. `stop` ceases combat; it does not log the
bot out. A reply saying "requested" confirms queuing, not completion. Normal
control belongs to the invitation-adopted controller while in the party; the
development GM override remains separate.

The port is moving from the temporary companion logic to upstream Playerbots'
engine, contexts, actions, triggers, values, and strategies. Imported components
and Cata-specific changes are tracked in [PORTING.md](PORTING.md). An imported
component is not necessarily wired into live bot decisions.

The bounded native character factory has passed its server-only check. Next are
broader donor feature ports after the infrastructure milestone. Managed MultiBot
roster/lifecycle and ordinary client creation/accounting also passed their bundled
check. The matching Linux server build and all 109 tests also passed. The core maintains
the [roadmap](https://github.com/iotadev/TrinityCore/blob/master/doc/local/playerbots/PLAYERBOTS_PORT_ROADMAP.md)
and [implementation handoff](https://github.com/iotadev/TrinityCore/blob/master/doc/local/playerbots/PLAYERBOTS_WORK_PACKETS.md).

The initial factory diagnostic is console-only:
`server playerbotdev managed appearance <race ID> <class ID> <gender 0|1>`.
It selects an appearance using Cata data and native validation without creating
an account or character. This preview does not certify account/name eligibility
or require managed admission to be enabled.

The default-off `Playerbots.Factory.InspectionEnabled` setting also enables
console `server playerbotdev managed inspect <account ID>`. It reads explicit
ownership evidence and native identities and reports a create/reuse/reject draft,
without writes or admission. See [optional schema and limits](sql/README.md).
`Playerbots.Factory.Enabled` separately enables console enrollment, provision and
factory-status. Enrollment dedicates a pre-created empty account; provision uses
native creation or exact-character accounting recovery on rerun. Both are
default-off, and neither automatically admits bots or grants player control.
The disposable server-only check passed schema rejection/application, enrollment,
native creation, exact reuse/accounting repair, conflicting-intent rejection and
explicit managed login/save/logout. A separate ordinary-player client check
validated creation/accounting and addon-managed connect/disconnect/reconnect.
Neither check certifies every addon/gameplay feature; see [validation details](PORTING.md).

## Building a matching development checkout

Place this repository at `modules/mod-playerbots` inside a **matching** core
checkout. The core build discovers it as an optional static module. From the
core root, configure with `-DMODULE_MOD_PLAYERBOTS=ON` (or `OFF` to exclude it).
Runtime bot admission is disabled by default.

The server/module is not designed to require Windows: its implementation uses
C++ and the core's native APIs, with ordinary CMake registration and Unix config
installation. Windows and Linux builds have been verified; the Linux checkpoint
used Ubuntu 22.04/GCC 11.4, both modules enabled and no precompiled headers,
and passed all 109 tests. **Linux server runtime has not been validated**.
Optional local PowerShell helpers are not
required to build or run the server. Keep them outside the portable runtime
contract rather than maintaining parallel shell implementations of each helper.

With the matching core's platform dependencies installed, the portable build
entry points are CMake and CTest, from the core root:

```text
cmake -S . -B build-portable -DMODULE_MOD_PLAYERBOTS=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-portable --config RelWithDebInfo --target worldserver tests-common
ctest --test-dir build-portable -C RelWithDebInfo --output-on-failure
```

These commands do not install dependencies, configure a database, or start a
realm. Generator/toolchain choices remain platform-specific. Repeat the Linux
build at relevant portability/publication checkpoints, not for every local edit.
The Lua addon candidate uses plain Git
and Lua 5.1 checks; it depends on the WoW client API, not PowerShell.

The module provides `conf/playerbots.conf.dist`. In the current development
layout, an active `modules/playerbots.conf` belongs beside `worldserver.conf`;
`Modules.ConfigDirectory` can select a different directory. Copy and edit the
template only in a disposable test realm. Building the module does not create
accounts or characters.

These are development notes, **not** complete installation instructions. This
repository alone cannot reproduce the tested server; use the matching core and
the separately versioned AHBot module if you want its economy behavior.

## Contributing and provenance

MultiBot server transport is available behind the default-off
`Playerbots.MultiBot.Enabled` option: versioned handshake, ping, authorized
live/offline managed rosters and managed connect/disconnect/status requests. Managed lifecycle
requires its separate enable/authorization settings. When player lifecycle
access is enabled it advertises only ALT_ROSTER_V1 and BOT_LIFECYCLE_V1;
other capabilities remain unsupported. The patched donor addon loaded without
Lua errors in the 2026-09-30 Windows Cata check; Stay, Follow and main Attack
worked. Managed roster/lifecycle UI, strategy and other feature families remain
unverified or unported. Attack currently has a temporary 25-yard owner/bot
target gate. See [addon preparation](addons/MultiBot/README.md).
Basic follow, stay/hold, attack and stop/cease also work through the donor's
ordinary bot-whisper and party/raid chat routes, with per-bot authorization.
Party chat is limited to the sender's subgroup when inside a raid.
An initial full-donor Cata compatibility candidate is reproducible through
[addons/MultiBot](addons/MultiBot/README.md). Its loading/transport patch has
syntax and mocked communication checks, not live client validation; it is not
a complete class/spec/talent-data port.

If you're working on Cata bots, compare notes with us. The most useful help
right now is reviewing Cata API differences or porting a class/spec from the
upstream module. Bug reports should name the core and module revisions, the
bot's class and level, and the behavior you saw. Keep credentials, client
files, database dumps, and private-server packages out of issues.

The retained source headers specify GPL version 2 or later; this port keeps
the license and author credits. [PORTING.md](PORTING.md) records the donor revision and the
changes needed for Cata. See also [LICENSE](LICENSE) and
[AUTHORS.md](AUTHORS.md).
