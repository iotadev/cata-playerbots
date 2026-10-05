# Cataclysm Playerbots

An experimental port of [AzerothCore mod-playerbots](https://github.com/mod-playerbots/mod-playerbots)
to TrinityCore Cataclysm 4.3.4. The goal is the familiar Playerbots feature set
on Cata, using upstream behavior wherever it fits the native server.

Use the matching [iotadev TrinityCore fork](https://github.com/iotadev/TrinityCore).
The module depends on that fork's session and module hooks; stock TrinityCore
does not provide them. The core README records its corresponding module revision.
Building this repository alone does not produce a server.

## Current capabilities

The current development target is a human-led Warrior/Mage/Priest party.

- Native bot sessions, bounded character creation, configured managed rosters
  and authorized connection/disconnection are implemented. Ordinary party
  invitations adopt the controller; lifecycle access has separate account-link
  and faction checks.
- Separate combat, noncombat and dead engines follow the donor state model.
  Control changes, target loss and state transitions discard obsolete queued work.
  Native map transfers, death and resurrection remain server-owned.
- Warrior Protection/Arms/Fury and Mage Frost/Fire/Arcane have starter combat
  routes. Priest support covers healing, buffs, disease cures, resurrection and
  conservative damage. Shared roles feed target selection, threat, positioning
  and support. Class/spec rotations and high-level spell coverage remain incomplete.
- Follow, hold, attack, stop, range controls, strategy queries and manual rebuff
  use ordinary whispers and party/raid chat. Party chat respects raid subgroups.
  Optional slices include saved-position stay, learned ground mounts, ready
  checks, carried recovery items and native corpse opening.
- The MultiBot bridge supports handshake/ping, authorized rosters and managed
  lifecycle requests. Strategy STATE framing and single-bot/group mutation
  acknowledgements use the same strategy-control layer as ordinary chat.

Implemented optional behavior requires its configuration gates and native
eligibility checks. A registered strategy or accepted cast does not establish
every spell effect, path or encounter outcome. Autonomous populations, questing,
travel, group formation and full dungeon/class parity remain future work.

## Validation

The current Windows and Linux development trees build worldserver and pass all
324 registered tests. Linux uses Ubuntu 22.04/GCC 11.4 with normal PCH enabled.
The core also passed a Windows build with both optional modules disabled and
19 core checks at the preceding milestone.
A separate GCC 13.3 protocol/group-policy check passed 30 cases and 4,380
assertions. Builds validated the milestone source before its final documentation
and commit. The later healer conservation, melee positioning and recovery metadata
slices passed on both platforms as an uncommitted source snapshot. Linux server
runtime has not been validated.

The 2026-10-04 outdoor party check observed all four bots engaging, role actions,
Mage damage, Priest Renew casts, return to noncombat and native corpse opening.
Client strategy queries showed group loot removal on all four bots. The realm
shut down cleanly. The quick encounter did not qualify sustained tank/healer
coordination. Aggregate addon ACK timing, framed state refresh, restoration and
the newer optional features still need observations in later party sessions.

The corrected 2026-10-05 Ragefire check confirmed dedicated entry for all four
bots into the same instance. The player reported working behavior over several
trash pulls; logs recorded Warrior role actions, Mage damage casts, Priest Renew,
return to noncombat and native corpse opening. All disposable services stopped
cleanly with no assertion found. This is a basic operational party milestone;
full clears, sustained healing/recovery and the optional timing checks remain open.

A longer October 5 Ragefire session observed eating/drinking, Mage/Priest drink
starts, repeated healing and tank aggro recovery, plus owner-death holding and
follow resumption after recovery. Services shut down cleanly. This accepts basic
recovery operation; it does not establish quantitative mana savings, detailed
tank orientation, Priest resurrection or a full dungeon clear.

The Lua 5.1 communication mock covers the installed Cata addon reader accepting
an ACK while pending and rejecting a late ACK after its timer expires.
[PORTING.md](PORTING.md) records donor revisions, Cata adaptations and dated
validation. The [archived README](docs/README_HISTORY_2026-10-04.md) preserves
earlier batch notes; it is historical context, not the current feature list.

## Build and configuration

Place the module at `modules/mod-playerbots` in the matching core checkout.
The core discovers it as an optional static module. With the core's platform
dependencies installed, build from the core root:

```text
cmake -S . -B build-portable -DMODULE_MOD_PLAYERBOTS=ON -DBUILD_TESTING=ON -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build-portable --config RelWithDebInfo --target worldserver tests-common
ctest --test-dir build-portable -C RelWithDebInfo --output-on-failure
```

These commands build source and run tests. Database setup and extracted client
data are separate prerequisites for running a realm. Optional PowerShell helpers
are development tools; the C++ module and CMake build do not require Windows.
AHBot is a separately selectable module in the same fork.

Copy [conf/playerbots.conf.dist](conf/playerbots.conf.dist) to the realm's module
config directory and configure it deliberately. The default layout places
`modules/playerbots.conf` beside `worldserver.conf`; `Modules.ConfigDirectory`
can select another location. Bot admission and optional features default off.
Building the module does not create accounts or characters.

The optional `Playerbots.Healing.SaveMana.Enabled` policy applies donor mana
conservation to the ported Priest heals. It defaults off. Its percentage estimates
guide spell selection; native Cata spell costs and healing remain authoritative.
It still needs sustained-party qualification before broader use.

## Chat and addon controls

Whisper an admitted bot, for example `/w Botmage list`, to list controllable
online bots. `follow`, `hold`, `stay`, `attack`, `stop` and `buff` use the same
authorization layer. Select a hostile target before requesting `attack`.
`stop` ceases combat; it leaves the bot logged in. A "requested" reply means
queued, and later responses report execution. Enabled `stay` holds a captured
position; its disabled fallback and `hold` use the existing plain stop behavior.

With `Playerbots.StrategyControl.Enabled`, `co ?`, `nc ?` and `de ?` query
registrations. Combat `focus/threat/potions` and noncombat `food/loot` support
`+`, `-` and `~`; role/spec strategies are protected, and dead state is query-only.
Changes require idle conditions and last for the current bot session. Global
feature settings still govern execution. A query can report "unchanged" because
it made no mutation; read the following strategy list for the actual state.

For the [Cata MultiBot addon candidate](addons/MultiBot/README.md), enable the
server bridge with `Playerbots.MultiBot.Enabled`. Structured strategy mutations
also require `Playerbots.StrategyControl.AddonMutations`; group requests require
the additional `Playerbots.StrategyControl.GroupMutations` gate. ALL selects
authorized controlled bots, GROUP/PARTY the current group including raids, and
RAID requires a raid. Execution rechecks control, login, group, phase and idle
conditions. Unresolved timeouts are unknown outcomes; refresh STATE before
retrying a toggle. Completed changes are not rolled back.

The full donor addon contains features the server has not ported. Its presence
does not imply class/talent data parity or support for every button.

## Contributing and provenance

Cata API comparisons, donor feature ports and repeatable bug reports are useful
contributions. Include the core/module revisions, bot class and level, and the
behavior observed. Keep credentials, client files and database dumps out of
reports. Follow [PORTING.md](PORTING.md) when documenting donor source and
adaptations so future upstream changes remain traceable.

Retained source headers specify GPL version 2 or later. This port preserves the
license and author credits; see [LICENSE](LICENSE) and [AUTHORS.md](AUTHORS.md).
