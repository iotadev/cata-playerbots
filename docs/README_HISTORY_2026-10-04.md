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

Local strategy controls now cover combat `focus`/`threat`/`potions` and noncombat
`food`/`loot`, through ordinary chat or opt-in MultiBot mutations. Changes
remain opt-in, safe-idle and session-local; role/spec strategies are protected.
Disabling the control gate restores class utility defaults, and global feature
settings still apply. Optional donor focus blocks non-healing area actions and
attacker debuffs; it is off by default. Existing Frost Nova now has hostile-area
metadata rather than being treated as a friendly self-buff, so focus and the
area-threat guard apply. No new AoE spell or targeting system is enabled.
Windows modules-enabled worldserver built and all 318 checks passed;
native chat/addon timing and newer Linux validation remain pending.

Group commands now connect authorized/controlled roster selection, bounded pending
batches and world-thread completion delivery. The separate default-off
`Playerbots.StrategyControl.GroupMutations` gate requires the base/addon gates.
ALL covers authorized controlled bots; GROUP/PARTY cover the current group including
raids; RAID requires a raid. Login/group/controller/security/phase/idle checks
revalidate at execution. Timeout results can remain unknown: refresh STATE before
retrying a toggle. No rollback or automatic retry is promised. Windows validation
does not establish native group-command timing or gameplay acceptance.

Local development adds donor secondary-caster interrupt routes for Warrior and
Mage. They can Pummel/Counterspell an engaged enemy's positive interruptible cast
without switching their current attack target or chasing it. Native spell checks
and the existing class-combat gates apply. This is not cross-bot interrupt
reservation or NPC-role inference. The final reviewed Windows build passed all 296
checks. Landed interrupts remain client-pending.

Local development adds shared combat recovery through carried healthstones and
healing/mana potions, behind default-off `Playerbots.Potions.Enabled`. It uses
donor thresholds of 25% health / 40% mana and prefers a usable healthstone before
falling back to a healing potion. Cata's native item-use path owns effects and
cooldowns, with no grants or direct restoration. Readiness counts the same supported
potion types; healthstones remain separate from its potion-stock requirement.
Flasks and channeled recovery remain outside this slice. Windows worldserver
built and all 295 checks passed for that slice; native effects/cooldown timing still need the
bundled party replay.

Local development closes the shared area-threat prerequisite: the existing PvE
attacker GUID list feeds the donor's 50% AoE threat guard, alongside the 80%
current-target guard. No new offensive AoE spell is enabled; area targeting and
pull safety still need their own donor dependencies. The latest Windows build
passed all 290 checks for that slice. Linux and bundled gameplay validation remain pending.

Local development adds structured MultiBot strategy acknowledgements behind
default-off `Playerbots.StrategyControl.AddonMutations`, requiring the base
strategy-control gate. Only single-bot C focus/threat/potions and N food/loot changes execute;
unsupported scopes and strategies reject. Completion follows map-thread execution
and snapshot publication, not queue admission. Overrides are session-local and
do not enable globally disabled rest/loot. That Windows slice built and passed all 288
checks passed. The current core also built with both optional modules disabled
and passed all 19 core-only checks.
Linux and client behavior remain unverified. This is not full strategy parity.

Local development adds read-only MultiBot strategy snapshots through the donor's
single-bot/global STATE framing protocol, under the same default-off strategy
control gate. Snapshots contain copied registrations published by the map thread;
the addon handler rechecks native roster authority and rejects stale/incomplete
responses. That read-only gate advertises `STATE_FRAMING_V1`; structured mutation
requires the separate option above. Neither provides self-bot parity.
The preceding Windows slice passed all 282 checks after review;
the current core also built with both optional modules disabled and passed 19
core-only checks. Client consumption/Linux acceptance remain unverified.

Local development exposes bounded ordinary strategy chat behind default-off
`Playerbots.StrategyControl.Enabled`: `co`, `nc`, `de` query registrations;
`co` focus/threat/potions and `nc` food/loot support add/remove/toggle. Requests cross a copied mailbox
and recheck current controller/authority on the map thread. Changes require idle
state and are session-local; roles/specs/stay remain protected, and global
recovery/loot settings still apply. Disabling the control gate restores utility
defaults without overriding roles/specs. The separate addon option above uses this same
engine/control layer. That preceding Windows slice passed all 277 checks. Its matching core
also built with Playerbots and AHBot disabled and passed 19 core-only checks.
Native gameplay and Linux validation remain pending.

Local source now wires donor-style saved-position `stay` behind default-off
`Playerbots.Movement.Stay.Enabled`. Enabled stay retains the current controller,
holds a captured ground position and uses native pathing to return after safe
nearby displacement. Follow/hold/control and lifecycle changes clear it; combat
and recovery yield. `hold` stays a plain stop, and disabled `stay` keeps the old
hold behavior. This is not combat-stay, ghost travel or persisted positioning.
That preceding stay slice passed Windows worldserver and all 267 checks. The regenerated current
core also built with both optional modules disabled and passed all 19 core-only
checks. Linux validation and bundled gameplay acceptance remain pending.

Local development adds optional learned ground-mount following, controlled by
default-off `Playerbots.Mount.Ground.Enabled`. It uses native riding/capability
and casting checks, pauses follow during its own mount cast and yields to
commands/combat/recovery. It does not grant mounts or add flight/travel autonomy.
That preceding mount slice passed Windows worldserver and 257 checks. The matching core also
built with both optional modules disabled and passed 19 core-only checks.
Linux validation and the bundled gameplay replay remain pending for this newer batch.

Local development now shares strategy-aware tank/healer/ranged roles across
party support, threat, target selection and positioning. Ordinary players use
native Cata specs; bots use their configured combat roles, including the current
Priest healer fallback. This does not add more bot classes or complete rotations.
That preceding role slice passed Windows worldserver and 252 checks;
its Linux and bundled gameplay acceptance also remain pending.

The current local batch now uses separate combat, noncombat and dead decision
engines, following the donor's state model. Transitions and control/target changes
discard queued work; native resurrection and transfer remain core-owned.
The saved shared-state snapshot passed Windows and Linux builds/tests; later
support/readiness changes still need Linux validation. The role-equipped party transition check
is pending, so this is not yet a client-accepted gameplay milestone.

A reusable level-20 Protection/Arms/Frost/Holy fixture can be prepared through
the default-off, console-only development tool. Native talents, equipped
weapons/shield and carried recovery items passed saved-data verification; it
does not enable autonomous character progression or population management.

Local development now includes a bounded Protection rotation port (Devastate,
Sunder Armor, Revenge and Sword and Board). These additions are not yet
client-confirmed; see PORTING.md for donor provenance and validation status.
The same Protection route now includes native Defensive Stance, Shield Block,
Shield Wall and Last Stand support, also pending bundled runtime confirmation.
Arms/Fury now have separate starter routes with Mortal Strike/Bloodthirst,
Execute and their respective stances. These remain incomplete rotations and
have not yet been client-confirmed.
The current single-target slice also responds to Arms Overpower/Taste for Blood
and Fury Bloodsurge/Slam. Proc handling remains native; runtime confirmation is
still pending with the rest of the spec batch.
Native Cata Colossus Smash and Enrage-gated Raging Blow are also scheduled
through those routes when learned. This does not establish full endgame support
or runtime-confirmed rotations.
Mage now has named Fire/Arcane starter routes alongside Frost and the generic
fallback. Arcane Blast, proc-gated Missiles and Barrage are implemented locally;
Broader Fire DoT support and native Arcane runtime confirmation remain pending.
Hot Streak Pyroblast now follows Cata's native spell override, and Scorch has
native-proc-metadata-gated Critical Mass support. Both are locally implemented
but runtime-unverified; Living Bomb remains deferred with AoE safety work.
Frost now schedules proc-only Frostfire Bolt and native frozen-state Deep Freeze,
with Ice Lance as the proc fallback. Native Frost runtime behavior is pending.
Mage self-defense now includes learned Mana Shield and Ice Block, plus Frost
Ice Barrier, through native casting. Absorb/immunity behavior remains unverified
in-game; this does not include Blink movement or early Ice Block cancellation.
Mage utility includes native self/party Remove Curse under the existing
Mage combat-engine flag, using shared party-support selection with Priest cure.
Native curse removal remains a bundled level-30-or-higher fixture check.
Learned Spellsteal is implemented for the existing controlled combat target,
excluding non-stealable auras; native buff transfer remains unverified in-game.

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

Manual rebuff uses the same ordinary whisper or party/raid
chat routes. Exact `buff` requests a bounded manual rebuff pass for supported
Mage/Priest party buffs, when their feature flag and controller are available.
It can top off aged buffs within the donor-default margin; it does not send a
native ready-check response or establish full-party readiness. The initial reply
means requested, and the later map-thread reply confirms the pass started.
Combat pauses the pass; death, transfer or controller changes cancel it.

Native `/readycheck` has a separate default-off `Playerbots.ReadyCheck.Enabled`
bridge. Bots check health, mana, proximity, idle/life/transfer state and usable
carried food/healing potions; mana users also require drink/mana potions. Missing
supplies mean not-ready. Group/check identity and a 30-second expiry protect replies.
Optional `Playerbots.ReadyCheck.ForceRebuff` waits for the existing supported
Mage/Priest buff pass, then rechecks readiness rather than assuming success.
Neither option establishes complete buff or encounter readiness. Windows
worldserver compiled and all 247 checks passed for that slice; the later
module-disabled Windows build also passed. Linux and client timing remain pending.

Range controls are available through the same ordinary whisper or party/raid
chat routes: `range ?`, `range spell ?`, `range heal ?`, `range spell 23`, or
`range heal 28`. Use zero to restore the default. Overrides are limited to
2–25 yards for spell positioning and 2–30 for healing positioning. Requests
are queued per bot and authorization is checked again on its map update;
the effective-range reply confirms the value was applied. These are positioning
settings, not permission to cast beyond a learned spell's native range. Active
Mage chases refresh when movement/casting guards permit. No new MultiBot button
or persistence is included. Client validation of this route is still pending.

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

Unpublished development now includes default-off `Playerbots.Rest.Enabled`:
supported companions can use carried native food/drink below the initial health
or mana thresholds, pause follow while recovering and resume afterward. No items
are created or purchased. Windows and Linux builds each passed all 113 tests.
Native execution has not yet been
client-confirmed. Keep this separate from the published infrastructure
milestone and its completed acceptance evidence.

The same unpublished batch adds default-off `Playerbots.Mage.Armor.Enabled`:
Mage self-armor follows the active spec and learned spells through donor-shaped
strategies. Arcane prefers Mage Armor; other/unspecialized bots prefer Molten
Armor, with learned Mage/Frost Armor fallbacks. Native casts own replacement;
no spells are granted. Windows and Linux builds each passed all 115 tests;
the bundled client check is pending for this addition. See `PORTING.md`.

Development also includes optional `Playerbots.Loot.PassOnGroupLoot`, using the
native auto-pass preference until equipment-aware rolling is ported. It applies
to new group rolls and restores the previous preference when disabled; it does
not loot corpses or choose upgrades. This needs the matching development core's
native pass-accounting correction, not the older published core pin. Native roll
resolution is still pending the bundled party check. See `PORTING.md` for limits.
The combined development batch builds on Windows and Linux and passes all 119
automated tests on each; these tests do not prove native loot awards.

The next local slice adds default-off `Playerbots.Loot.Corpses.Enabled` for
nearby defeated or player-selected corpses, with an eight-entry expiring list
and short native-pathfinding detours. It uses the matching core's typed loot result, permissions,
money/item storage and release on the world thread, with bounded attempt backoff.
It does not roam for distant loot, scan an area, gather, skin, collect currencies
or evaluate upgrades. Windows and Linux builds each passed all 123 tests;
the bundled outdoor check confirmed native corpse opening by all four bots
and clean shutdown, not actual item awards or group-roll resolution.
The older published core lacks its result seam.
The movement follow-up remains unverified in game; detours are bounded to ten
seconds and yield to combat, commands and transfer. Rest/armor defer during them.
Its final Windows and Linux builds each passed all 126 automated tests.

Implemented Warrior and Mage combat routes now follow the current active
specialization without requiring a bot reconnect. This updates Protection/generic
Warrior and Frost/generic Mage selection; it does not supply complete rotations
for the other specs. Priest still uses the current healing fallback. Shared
buff/rest/loot strategies survive changes, while obsolete queued combat work
is cleared. Windows and Linux each passed all 129 tests; live spec transitions
remain unverified for this follow-up.

The same combat-engine flags now include known Pummel and Counterspell against
the current target's interruptible cast or channel, using native cast rules.
That initial slice had no enemy-healer scan; the secondary-caster routes above
extend it without spell grants or cross-bot interrupt reservation.
Mage currently finishes its own cast rather than cancelling it to interrupt.
Windows and Linux each passed all 132 tests; landed interrupts remain
unverified for this slice.

Warrior now has a known Heroic Strike rage spender with donor medium/high-rage
reserves. The Frost Mage route can prioritize learned Ice Lance for frozen
targets or an effective Fingers of Frost proc. Both use native spell execution;
generic Mage remains unchanged. This is a bounded Cata adaptation, not a complete
rotation or unrestricted Ice Lance movement filler. Windows and Linux builds
each passed all 135 tests; live resource/proc effects remain unverified.

Priest's healing-engine route now includes self/party Cure Disease when learned
and natively eligible. It preserves native dispel protections and uses the donor
disease-cure priorities. Cata lacks Wrath's Abolish Disease; magic dispels and
talent-dependent extensions are not included. Windows and Linux each passed all 138 tests;
actual disease removal remains unverified in game.
The subsequent targeting correction keeps healthy/full-health party members
eligible for cures while preserving the healing-only health cutoff. Windows
and Linux each passed all 141 tests for that correction.

The healing-engine route also includes conservative healer damage. The Priest
uses the existing controller/target commands, pauses follow without melee/chase,
and restores formation when support ends. Damage requires controller combat,
eligible nearby party health >=90% and balance-dependent mana reserves (below);
healing and cures have priority.
This is not a full healer rotation or donor group-balance policy. Windows and
Linux each passed all 145 tests for the initial Smite slice. The follow-up adds
Shadow Word: Pain, Holy Fire and Mind Blast in donor order, with native
caster-owned DoT checks. Windows and Linux each passed all 147 tests. The shared
combat-value follow-up now uses the donor level/gear/role estimate for Shadow
Word: Pain's eight-second lifetime gate. Profiles cover supported bots at levels
1–80; an unavailable estimate skips that DoT. This is not measured damage or a
calibrated Cata 81–85 profile. Windows and Linux each passed all 154 tests; bundled
gameplay checks remain.

Scheduled Mage/Priest single-target damage also has a donor-style threat guard:
it yields at 80% of a recognized group Protection Warrior's threat. Human tanks
count; absent a recognized tank, it does not throttle damage. Healing and other
support remain available. That initial slice did not cover AoE; the newer area-threat
prerequisite above adds its shared guard without enabling area spells. Warrior
auto-attacks and direct/manual action execution remain outside this multiplier.
Windows and Linux each built and passed all 159
tests; bundled gameplay confirmation is pending.

The engaged PvE attackers/balance follow-up reads native party threat references
without selecting new targets. Priest damage uses donor-default mana thresholds
85/65/40 according to the level/rank balance estimate; healing/control checks
remain in force. This is not measured difficulty or full attacker/target-selection
parity. Windows and Linux each built worldserver/tests-common and passed all 162
tests; bundled gameplay checks remain pending.

Idle Mage/Priest auto-assist now has a bounded donor-style target fallback when
the player has no valid selected combat target. It ranks only creatures already
fighting the controller, with strategy exclusions and marker priorities. Explicit
commands, valid selections and active attacks keep precedence; Warrior targeting
is unchanged. No autonomous pulls or full target-selection parity are claimed.
Windows and Linux each built and passed all 167 tests; bundled gameplay is pending.
The follow-up adds named GUID `dps target` / `tank target` services and idle
Protection Warrior fallback under EngineWarriorCombat. Tank ranking prioritizes
lost aggro, then melee/range and own threat. Active switching, taunts and broader
roles/main-tank coordination are unported. Windows and Linux follow-up builds
each passed all 170 tests; native gameplay checks remain pending.
The subsequent source correction also covers idle fallback for unassigned/DPS
Warriors, using general DPS ranking with native melee reach. Protection keeps
tank ranking; commands, active attacks and the leash are unchanged. Windows and
Linux each built and passed all 171 tests; live verification remains pending.

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
