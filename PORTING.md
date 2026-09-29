# Port provenance and remaining work

Current status, 2026-09-29: the scheduling kernel, session adapter, bounded
Warrior/Mage/Priest contexts, active roster and normal whisper controls are
integrated. The release check below confirmed Mage offensive casts, Priest
healing and basic commands. The dated entries describe their state when added;
older "pending" wording is superseded only for the behavior explicitly tested.

The recorded local donor `8827dd6fcbb2bb25988787a40f06fc93daf8e02d` contains
upstream master `7bae1b5c58c76a0aa20381155edc08096d1485b2` plus five local
commits. The upstream commit is available from the public donor repository;
the local custom revision is not assumed to exist in that repository. Check
upstream master before new imports and document local additions separately.

Foundation extraction: 2026-09-26, from this fork's existing dirty Cata prototype
on base `efcf6ac83d11fdf4ce86a1b6f95c3b22dfaee14f`. The preserved pre-extraction
files are under ignored `build/module-foundation-baseline-20260926`. Source notices
remain on the moved helpers. The local GPLv2 mod-playerbots donor is pinned at
`8827dd6fcbb2bb25988787a40f06fc93daf8e02d`; no wholesale engine import occurred in
this foundation change.

Current donor relationships are adaptations of mechanisms, not source parity:

| Cata prototype | Upstream responsibility |
|---|---|
| Party invitation handling | `src/Ai/Base/Actions/AcceptInvitationAction.cpp` |
| Follow positions/catch-up and melee positioning | Base follow actions and movement/formation values |
| Starter class priorities | `src/Ai/Class/{Warrior,Mage,Priest}` strategies/actions |
| Spell attempts | Native Cata Spell validation; to become the imported cast action adapter |
| Session behavior bridge | Replacement seam for donor PlayerbotAI/context/state engines |

The extraction retains map-update behavior and world-update transfer acknowledgement.
After a worldport acknowledgement, it resolves the current Player again. It removes
one ineffective remembered-owner assignment immediately before a queued hold that
would clear that value anyway. No class priorities, spell IDs, health thresholds,
follow distances, invitation security policy or dungeon entry policy are changed.

Still to port: donor Engine/context registrations, packet/event adapters, normal
chat/security/controller policy, active-spec profiles, factories, managed population,
world progression and optional module APIs. Preserve names and responsibilities
while recording each Cata-specific change and original author/source when code is
actually imported. The module directory alone is not full upstream compatibility.

## First upstream registry import - 2026-09-26

Imported `src/Bot/Engine/NamedObjectContext.{h,cpp}` from the pinned donor above.
This is actual donor code, not a similarly named reimplementation. The upstream
notices are preserved, and the donor `AUTHORS.md` is retained here. Donor history
for this file includes Keleborn, NoxMax, Crow and bashermens; its latest modifying
commit is `917a22bc` by Keleborn. This is provenance, not a claim that one author
owns the entire file. Preserve donor authorship in any later publication/commit.

Cata deviations:

- Replace the broad `Playerbots.h` include with direct standard headers; keep
  native `Common.h` types. No full runtime, database, travel or population
  dependencies are needed for this registry slice.
- Delete copy construction/assignment for the four owning context/factory
  containers. A copied owning raw-pointer cache would double-delete its objects.
  Creator signatures, names, qualifiers, overrides, sibling lookup and cache
  behavior remain donor-compatible. These containers are not moveable either.
- Use `size_t`, not `uint8`, in `MultiQualify` iteration so 256+ components
  cannot wrap the loop counter. No broad qualifier parsing redesign is included.

Registration ownership remains upstream-shaped: shared lists own creator
contexts; each per-bot list owns its created objects; action-node factory list
results are uncached and caller-owned. Complete registration before map-thread
use, and keep shared lists alive until all their per-bot lists are destroyed.
No live registration mutation or cross-thread object sharing is added.

Seven new registry tests exercise qualification, separate per-bot caches, sibling
groups, later creator overrides, clear/destruction ownership, caller-owned nodes
and >255-component formatting. Enabled worldserver/tests-common build and all
41 CTest cases passed. The donor codestyle checker passed on an isolated copy of
the three new C++ files. clang-format was unavailable locally.

This registry is not wired into active gameplay yet. Next: import the event,
action/trigger/value/strategy and relevance queue dependencies, then Engine and
the per-bot state runtime. Activate one decision owner only after the closure
builds and lifetime/event adapters are reviewed.

## Upstream Event and NextAction import - 2026-09-26

Imported `Bot/Engine/WorldPacket/Event.{h,cpp}` and extracted `NextAction` from
`Bot/Engine/Action/Action.h` at the same donor pin. Original notices are preserved.
Latest modifying donor commits inspected: Event.h `9f386de7` (Keleborn), Event.cpp
`9e8fe5e5` (NoxMax), Action.h `dfbbbf84` (Keleborn). The module's retained AUTHORS.md
continues to supply full donor attribution; these commit names are provenance,
not exclusive authorship claims.

Cata deviations:

- Replace umbrella dependencies with native packet/GUID and standard headers.
- Keep the upstream Player-pointer constructor surface, but capture the owner's
  GUID immediately. Add GUID overloads for queued producers. `EventOwner.cpp`
  isolates native pointer capture and ObjectAccessor::FindPlayer resolution.
  No Player pointer is retained in the event, and no socket/client-origin session
  or authorization bypass is introduced. Resolved pointers are borrowed for the
  current call only; callers must obey native execution-context/lifetime rules.
- Native Cata uncompressed GUID serialization is uint64. `getObject` reads from
  an independent payload copy starting at zero, preserving the event's cursor;
  short payloads return an empty GUID. This is an internal GUID event contract,
  not a WotLK packet decoder or general validation of incoming Cata packets.
- Extract NextAction without importing incomplete Action/AiObject/Value classes.
  Name, relevance, ordered concatenation and duplicate retention are preserved.

Six new tests cover empty/named events, command copies, independent packet data
and cursors, owner/target identity, short GUID payloads, and NextAction merging.
Tests compile real native packet/GUID types through `game-interface`, but omit
the native owner adapter: no fake Player or replacement ObjectAccessor is used.
FindPlayer's in-world filter is source-inspected and the adapter compiles into
worldserver; logout resolution and cross-thread safety are not runtime-proven here.

Enabled RelWithDebInfo worldserver/tests-common builds and 47/47 CTest cases pass.
The initial test compile found ambiguous integer GUID constructors; fixtures now
use explicit uint32 values. Donor codestyle checks pass on the five new C++ files.
The module-off build was not rerun in this slice; optional test sources/libraries
are registered only in the enabled module branch. No server, DB, client, gameplay
decision owner, class priority or donor source was changed by this verification.

Next: complete the AiObject/value/action/trigger/strategy/relevance-queue dependency
closure and per-bot context before activating Engine. Events are not yet wired
to native chat or outgoing packet producers.

## Scheduling kernel import - 2026-09-27

The next donor slice imports the scheduling behavior from
`Engine/Action/Action.{h,cpp}`, `Engine/Trigger/Trigger.{h,cpp}`,
`Engine/Value/Value.h`, `Engine/Strategy/Strategy.{h,cpp}`,
`Engine/PlayerbotAIAware.h`, `Engine/AiObjectContext.{h,cpp}`, and
`Script/WorldThr/Queue.{h,cpp}` at the
same pinned donor revision. The default Strategy action-node factory is copied
from the donor; it retains names and fallback links. The code is built into the
optional module but is not yet the live decision owner.

This is the runtime-independent scheduling portion, not the complete upstream
classes. `Action`, `Trigger`, and `UntypedValue` retain donor-facing names
and scheduling methods but do not yet inherit `AiNamedObject`; player-bound
target/context/chat access waits for the per-bot `PlayerbotAI` and
live-context lifetime seam. `AiObjectContext` now owns separate per-bot
strategy, action, trigger, and typed-value registries. Its shared creator
tables are deliberately empty until class contexts are ported. Specialized
Unit, CreatureData and GUID
values, memory/log calculated values, performance monitoring and the full
`Engine` are also still absent. Donor class action/trigger implementations
cannot be dropped in unchanged yet.

Cata/local adaptations in this slice:

- `Event` remains the earlier GUID-owned Cata event, so queued baskets do
  not retain an owner Player pointer.
- The queue receives action expiry as a setting rather than consulting the
  donor global `sPlayerbotAIConfig`; future Engine setup must supply it.
  Its destructor and `Clear` release pending nodes. `Pop` transfers a node
  to the caller, and duplicate names keep the first event while raising
  relevance, matching donor behavior. Negative relevance can still be popped
  rather than becoming an unreachable queued node.
- Trigger force-rebuff inputs are supplied at `needCheck` by the future
  runtime, rather than dereferencing a missing donor `PlayerbotAI`. The
  out-of-combat buff-trigger bypass is retained.
- Cached values use Cata's native millisecond timer without donor
  PerfMonitor. A separate cache-valid flag avoids recalculating repeatedly
  when the first read occurs at millisecond zero.
- Unbound action nodes return their configured handlers instead of
  dereferencing a null Action. Real Engine registration must still bind
  actions before execution.

Focused tests cover ordering/duplicates, queue ownership, strategy fallback
links, trigger timing/rebuff gates, value cache/reset behavior, per-bot
registry isolation, typed lookup, and value save/load. This is a
build-verified preparatory slice; it does not change combat rotations or prove
the forthcoming `PlayerbotAI`/Engine lifetime bridge.

## Decision engine import - 2026-09-27

`Engine.{h,cpp}` and `Multiplier.h` now carry the donor's strategy assembly,
trigger/default-action scheduling, action-node factories, relevance multipliers,
prerequisites, alternatives, continuers, direct action execution, and execution
listeners. They use the context and queue above. Focused tests exercise action
ordering, failure fallback, trigger firing, and listener vetoes.

The Cata adapter takes a per-bot context and supplies queue expiry, rebuff
state, combat state, and iteration budget at its boundary. It does not
dereference `PlayerbotAI`, write donor perf logs, or own a `Player`/session.
The trigger interval is not reset after every tick, so the interval gate can
actually throttle checks. A minimal tick stops when the highest queued action
is below its threshold rather than looping on the same basket. Engine owns
its nodes, multipliers, and listeners; the context owns Action, Trigger, and
Strategy instances.

This engine is compiled but is **not active in gameplay**. At this milestone
the shared class creator tables were still empty. No Cata core hook was
changed in this slice.

## Session adapter and first Warrior context - 2026-09-27

The session-owned Cata `PlayerbotAI` adapter now gives donor-facing objects a
`GetBot` and current-target lookup. It stores a `WorldSession` reference and a
target GUID, not a `Player*`. `GetBot` resolves the current in-world Player and
checks the server-origin character GUID. Access is map-thread only.
`PlayerbotSessionBehavior` constructs the adapter before login, then lazily
creates the class context and engine when a Warrior Player is available. The
engine is destroyed before its context and adapter. Target GUID changes track
the current attack/cease path. The existing companion movement, healing, and
combat code remains the live owner. By default the old buff call also remains
active; `Playerbots.Dev.EngineWarriorBuff = 1` swaps only the out-of-combat
Warrior Battle Shout maintenance call to the new engine for a narrow live test.

`WarriorAiObjectContext` follows the donor's shared-creator/per-bot-cache
pattern for the first `nc` strategy, `battle shout` trigger, and `battle shout`
action. Names and trigger/action relationships come from the donor; the
Warrior's 6673 spell/aura check and cast use the existing Cata spell adapter.
This is a foothold, not the full upstream Warrior context or rotation. The
server build compiles the live Cata code. Unit tests do not link native Player
and spell-casting implementations, so the real cast required the gated live
test recorded below.

## Gated Warrior buff runtime check - 2026-09-28

A disposable local realm with `Playerbots.Dev.EngineWarriorBuff = 1` recorded
the engine route, an accepted Battle Shout cast on Testone, and auto-assist
engagement/return-to-follow during nearby combat. World/auth/MySQL shut down
cleanly. The user also saw Battle Shout in the client. This validates the
narrow live route; no broader Warrior rotation was enabled.

## Bounded Mage engine profile - 2026-09-28

`MageAiObjectContext` now registers donor-shaped `mage` and `frost` combat
strategies, an `enemy is close` trigger, and Cata-native Frost Nova, Fire Blast,
Frostbolt and Fireball actions. The active Cata talent tree selects `frost` only
for `TALENT_TREE_MAGE_FROST`; an unset, Arcane or Fire tree uses a deliberately
generic fallback, not a claimed full specialization rotation. The shared Cata
cast adapter still decides learned spell, resource, cooldown and cast legality.
The session switches the Mage combat decision owner only when the default-off
`Playerbots.Dev.EngineMageCombat` option is enabled.

This is build-verified, not gameplay-verified. CMake was regenerated to include
the new source, worldserver built successfully, and the existing 65 tests passed.
No client test or Mage gameplay claim follows from this patch. Batch-test it
with subsequent Priest/group work.

## Bounded Priest healing engine and living-party recovery - 2026-09-28

`PriestAiObjectContext` registers the donor's `heal` strategy, party-health
trigger names, and party-targeted Shield, Flash Heal, Heal and Renew actions.
The actions use the Cata spell validator and the existing lowest-health-first
candidate rule. `Heal` spell 2050 replaces the donor's WotLK Greater Heal
assumption in this narrow level range. The engine route is default-off behind
`Playerbots.Dev.EnginePriestHeal`; normal companion healing remains the default.

The session adapter now keeps a controller GUID rather than a Player pointer.
When the controller dies, the Priest can still consider living reachable group
members and itself while recovery is pending. An explicit hold clears the
controller and stops that automatic healing. This is not resurrection,
mana/rest management, or a complete Discipline/Holy/Shadow profile. The source
compiled into worldserver and all 65 existing tests passed; the gated healing
route and owner-death behavior are not client-tested yet. Combine those checks
with Mage and group control in the next representative party playtest.

## Invitation controller lifetime - 2026-09-28

An accepted human party invitation now records an invitation-owned controller
separately from the console-only follow command. The map update checks that
the inviter remains a party member; if the bot leaves or is removed, or the
inviter leaves, it requests hold and drops the party controller. Explicit
console follow overrides this lease and remains a development-only path.
Recovery after death and the existing dungeon transfer retain the invitation
lease when they reissue follow. This maps the donor's accept/leave-group
actions onto the current Cata session bridge; it is not the donor's complete
`PlayerbotSecurity`, new-master election, or normal chat command surface.
Compile verification is separate from the pending mixed-party client test.

## Priest out-of-combat resurrection - 2026-09-28

The donor's `PriestNonCombatStrategy`/`party member dead`/`resurrection`
relationship is now represented in the Priest context. The action attempts
Cata Priest Resurrection (2006) only for a nearby, visible, dead party member
with no outstanding request while the Priest is alive and out of combat. It
uses the existing Cata spell cast validator; no direct player revival is done.
When a server-origin party bot receives a resurrection request from a current
living party member, its session answers through Cata's normal resurrection
response handler, which retains the core's request and raid-charge checks.
Both parts are covered by the default-off `Playerbots.Dev.EnginePriestHeal`
switch. This is source/build validation pending the mixed-party client test;
released ghosts outside range, movement-to-corpse, and complete dead-state
strategy parity are not solved by this slice.

## Bounded Warrior combat engine - 2026-09-28

`WarriorAiObjectContext` now also registers donor-shaped generic `warrior` and
Protection `tank` combat strategies, with mutually exclusive combat strategy
selection by the active Cata talent tree. Both retain the current low-level
Victory Rush proc, Rend upkeep and Strike fallback. The Protection variant
adds Shield Slam when learned and Taunt only when the current creature targets
a party member other than the Warrior. All casts pass through Cata's native
spell validator; spell IDs are 34428, 772, 88161, 23922 and 355.

The new combat decision owner is default-off behind
`Playerbots.Dev.EngineWarriorCombat`; the existing Warrior combat helper
remains live otherwise. It is independent of the separately gated Battle
Shout route. This is not a complete Cata Protection rotation, stance manager,
pull strategy, multi-target threat planner or proof of dungeon tanking. The
next mixed-party playtest should exercise the gated Warrior, Mage and Priest
paths together instead of adding another one-spell client check.
The worldserver compiled and all 65 existing tests passed; none of those
tests establish actual Warrior damage or threat behavior in the client.

## Party-member death recovery - 2026-09-28

The follow-resume guard now requires the adopted human controller to remain
in the bot's party, rather than requiring that human to still be its leader.
This matches the invitation controller lease and permits leadership changes
without stranding a recovered bot. It still requires both players alive,
nearby, on the same map, and an actual human controller. The next party test
must verify resume after resurrection and hold after party removal.

## Mage ranged opener correction - 2026-09-29

The mixed-party Ragefire run routed Botmage through the new engine but logged
zero accepted offensive casts. The Mage action's target predicate required
`IsInCombat()` before the first spell. In this Cata core, a player's ranged
`Attack(target, false)` selects an attack target without itself entering combat;
the opening spell is what establishes combat. The engine predicate now allows
that opening cast while keeping native attack-target, distance, line-of-sight,
facing, learned-spell and cast-legality checks. This is a source-level correction
pending a later batched client check, not proof that the Mage rotation works.

## First PlayerbotSecurity relationship slice - 2026-09-29

`Mgr/Security/PlayerbotSecurity.{h,cpp}` adapts the donor security-level names
and its distinction between invitation and full control. The pinned donor is
`src/Mgr/Security/PlayerbotSecurity.{h,cpp}` at
`8827dd6fcbb2bb25988787a40f06fc93daf8e02d`; original notices and
`AUTHORS.md` remain. Recent donor history for those files includes NoxMax,
Keleborn and Crow; this is provenance, not exclusive authorship. The Cata
class evaluates current `Player`/session state
without retaining pointers or making a map-thread database query. The session
bridge exposes the invitation-adopted controller GUID to the policy.

This slice deliberately does not import the donor's random-account, gearscore,
guild, linked-account, battleground or chat-denial branches: their supporting
manager/config/data APIs have not been ported. For the existing development
roster, a normal same-faction human may invite an ungrouped bot, while only the
current human invitation controller in the bot's party receives full control.
Other group members receive talk level. The donor's GM override is retained.
The existing invitation flow now checks the invite level before accepting;
normal core party checks still decide whether the invitation can be formed.
The four-slot dev commands remain console-only and are not an addon-control
surface. Subsequent chat/bridge commands must call this same policy, not trust
client-provided bot names or group membership alone.

## Identity lookup and control dispatch - 2026-09-29

`World::FindServerOriginPlayerbot` resolves an active session by the server's
character GUID and verifies server origin; the old numbered slot lookup now
uses it plus the configured account check. This is a world-thread lookup over
active sessions, not a managed character roster or a character factory.

`Bot/Cmd/PlayerbotControl` provides a world-thread command boundary for a
future chat or MultiBot transport. It resolves that GUID, requires the full
`PlayerbotSecurity` level, and queues follow, hold, attack or cease into the
existing session mailbox. Attack requires the bot to be following the
requester. Separate party-controller follow/hold requests preserve the
invitation lease, unlike the explicit console overrides. At this step no addon
packet or chat command called the dispatcher; the following whisper transport
now uses it. A managed roster, richer donor security, the full donor command
parser and Cata addon-message handling still need their own ports.

## Active roster and first normal command transport - 2026-09-29

`PlayerbotRoster::ListActiveFor` enumerates current server-origin sessions and
returns only in-world bots for which `PlayerbotSecurity` grants the requester
full control. Entries carry server GUID, name, class and level and are sorted
for stable presentation. This is an active roster, not the donor's offline
account/character manager, factory, guild/friend roster, or random population.
It deliberately does not infer ownership from a shared group alone.

A PlayerScript observes ordinary whispers to admitted bots. Exact `follow`,
`stay`/`hold`, `attack`, and `stop`/`cease` words use `PlayerbotControl`; `list`
shows the requester's active controllable bots. Other whispers and addon-language
messages are untouched. Core chat checks still run before the script, and the
handler is registered only with the optional module. The command vocabulary is
a narrow Cata transport mapped to upstream Playerbots concepts, not a port of
the donor's full command parser. The addon bridge should consume the same
roster/control services through structured requests, not parse these replies.

## Integrated client checkpoint - 2026-09-29

The combined worldserver build passed before this check; the preceding existing
automated suite passed 65 cases. With the Warrior-combat, Mage-combat and
Priest-heal engine routes enabled in a disposable realm, four bots accepted
invitations and entered Ragefire Chasm. Logs recorded 13 accepted Mage offensive
casts, six Priest healing casts and one Fortitude cast. The human reported good
overall behavior and basic whisper/movement controls; all test processes shut
down cleanly after logout. This confirms the Mage opener correction and bounded
healing route. Warrior routing was observed without establishing tank threat.
The fixture human had GM privileges; this session did not independently verify
ordinary-player authorization/denial boundaries.

One attack request after stop was rejected with the bot selected instead of a
hostile unit. A later explicit Earthborer attack and casts were logged; the
specific resume interaction remains a command-UX follow-up. Resurrection,
controller death, removal/re-invite, full rotations and a dungeon clear were
not verified in this session. No additional playtest is required merely to
continue the manager/roster and addon infrastructure work.
