# Port provenance and remaining work

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
and spell-casting implementations, so the real cast still needs a gated live
test with that switch enabled.
