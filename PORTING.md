# Port provenance and remaining work

## Shared decision states — 2026-10-02

Audited upstream master at `037c01418b5d01506917a3db9b44fd56ac5f965c`:
`src/Bot/PlayerbotAI.cpp` engine construction/ChangeEngine and
`src/Bot/Factory/AiFactory.cpp` combat/noncombat/dead factory registrations.
Compared against previous `7bae1b5c58c76a0aa20381155edc08096d1485b2`; the newer
AI changes do not alter this pattern. Existing spell/action imports retain
their own pinned provenance, not a retroactive master label.

`StateEngines` adapts the separate-engine ownership and active-engine switch to
the existing shared Cata context. State queues/strategy sets are separate.
The map-thread bridge selects native death, native combat/authorized engagement
or noncombat. State switches clear pending prerequisites/continuers in all
engines; steady-state selection preserves them. Stop/movement/target changes
clear pending work without requiring native combat flags to drain first.
Transfers suspend ticks and invalidate pending work. Spec refresh affects only
combat registrations. Priest support remains registered in both live states.

The dead engine is intentionally empty: native resurrection and follow recovery
retain the existing session ownership. Donor ghost travel, automatic graveyard
release and autonomous lifecycle behavior are not claimed. No database schema,
thread scheduler or production fixture-grant interface was added.

Windows worldserver/tests-common built and passed 195 registered checks.
Linux GCC 11 regenerated, built both targets and ran 195 Catch cases: 194 passed, one failed
as expected, no unexpected failures. This closes the prior outstanding Linux
curse/Spellsteal compile validation. Real queue/state regression tests passed;
native client transition wiring and role-qualified gameplay remain pending.
See the matching core's shared-state acceptance and repeatable party fixture.

### Native disposable fixture

Added default-off `Playerbots.Dev.Fixture20.Enabled` and a console-only
configured-slot `fixture20` request. A bounded GUID/role mailbox is consumed
on the existing map update; no Player pointer or new core scheduling seam is
introduced. Native LearnPrimaryTalentSpecialization/LearnTalent select the
level-20 Protection/Arms/Frost/Holy roles and spend only available points.
Native spell/inventory checks grant explicit test skills, preserve old equipped
items in bags, equip modest verified items and top up food/water. Completion
is separate from command acceptance and SaveToDB requests are not DB commit proof.
The tool rejects wrong class/tree/level, grouped, dead, combat or transfer states.
This is test scaffolding, not donor production character/equipment progression.
The existing harness can prepare/save/stop the fixture without a client.

Server-only preparation passed on 2026-10-02. The four native primary trees were
845/746/823/813 with zero free talent points. After native logout the harness
verified saved talent records, level, equipped weapons and Protection shield,
plus carried food/water. World/auth/database shut down cleanly. The stopped
copy is retained only as ignored local fixture data. Client combat/transition
acceptance is deferred by the user; the milestone candidate is not full gameplay
acceptance. The fixture audit caught absent legacy spell 168; it was removed.
Native Cata armor records start above this fixture's level and await later tests.

## Reading this history

Entries below record evidence at their dated revisions, newest first. Older
"pending" or inactive-engine statements are not the current project status;
later entries supersede them only for the behavior actually verified. Use the
module README and the matching core's roadmap and infrastructure acceptance
checklist for current scope. Windows build/tests and the server-only factory
batch have passed. Ordinary-player addon lifecycle also passed the bundled
check below. Linux build and automated tests passed as recorded here; Linux
runtime remains unverified. No full donor feature parity is claimed.

## Mage current-target Spellsteal - 2026-10-02, development

Adapt donor GenericMageStrategy spellsteal priority 40 and Mage SpellstealTrigger
at `7bae1b5c58c76a0aa20381155edc08096d1485b2`. Cata learned spell 30449 has
native steal-beneficial-buff effect 126, magic type 1 and learn level 70. The
current validated hostile Creature supplies native dispellable magic candidates;
exclude CANNOT_BE_STOLEN in addition to the native passive/polarity/resistance/
charge filters. Trigger and action recheck eligibility, retaining no aura pointer.
Native Spellsteal owns random selection, resistance, removal/transfer, charges,
duration and cost. Existing controlled targeting, facing/LOS/range and conservative
single-target threat guard remain. No enemy search, PvP routing, general purge,
manual aura transfer or new permission. Two policy tests added; Windows
worldserver/tests-common built and all 192 CTest cases passed. Core/module diff
checks passed. Docker still returns an internal API error; current Linux validation is
unavailable, including the previous curse slice. See research/SPELLSTEAL_PACKET.md.
Native buff transfer remains unverified.

## Mage curse utility and shared party support - 2026-10-01, development

Adapt donor MageCureStrategy and RemoveCurseTrigger/PartyMemberRemoveCurseTrigger
at `7bae1b5c58c76a0aa20381155edc08096d1485b2`: self/party Remove Curse at 41/40.
Cata spell 475 is a native curse dispel learned at level 30. Existing Priest
candidate gathering/unfiltered stable ordering now live in shared party support;
Priest wrappers retain their class/healing filters and native disease behavior.
Shared native dispel eligibility verifies the requested spell's dispel effect
type before consulting GetDispellableAuraList. No retained aura pointers or
manual removal. Mage cures require its existing default-off combat-engine flag,
an alive nearby controller, learned spell and native eligible friendly player.
Rest/loot/transfer/mounted/casting work defers cures. Register independent cure
strategy in both combat/noncombat states and reuse the existing passive timer
for enabled Mages; no new timer, scheduler, lifecycle or database seam. Other
class passive-tick gating remains unchanged. Three new policy/strategy tests;
Windows worldserver/tests-common built and all 190 CTest cases passed, including
existing Priest regressions. Core/module diff checks passed. Linux build was
started, but Docker's API returned an internal error during status inspection;
completion cannot currently be confirmed. Do not infer Linux validation from
the preceding 187-case batch. See research/MAGE_CURSE_PACKET.md. Native
curse removal and resistance/protection cases remain runtime-unverified.

## Mage defensive support - 2026-10-01, development

Adapt donor GenericMageStrategy.cpp and FrostMageStrategy.cpp at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: Ice Block 90 below 25 percent,
Mana Shield 85 below 45 percent, Frost Ice Barrier 29 below 65 percent or
attacked by the current enemy. These are donor default thresholds, not a port
of its configurable health values. Actions recheck learned spells/current aura
and native exclusion metadata (Ice Block Hypothermia), then request native self
casts. Self-defense is not enemy-facing/LOS gated, but remains in the existing
controlled combat/session path. No own-cast cancellation, Blink, early Ice Block
removal, aura injection or absorb/cost duplication. Native glyph/talent side
effects remain native, including reactive area effects; no new offensive AoE
strategy is added. Three policy tests added. Linux worldserver/tests-common
built successfully; 187 cases completed with 186 passed and the existing
expected failure. Windows worldserver/tests-common built and all 187 CTest
cases passed. Core/module diff checks passed.
See research/MAGE_DEFENSIVE_PACKET.md. Native effects remain runtime-unverified.

## Frost proc follow-up - 2026-10-01, development

Adapt donor FrostMageStrategy.cpp at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: Brain Freeze -> Frostfire Bolt and
frozen/Fingers of Frost eligibility -> Deep Freeze. Cata aura 57761 effect 1
must affect learned Frostfire Bolt 44614 and fully reduce cast time. Deep Freeze
44572 reads its native frozen target requirement through HasAuraState, including
affecting native ignore-state effects. No manual proc removal or damage/stun.
Priorities 23/22 preserve donor Brain Freeze-before-Deep Freeze order while
placing both above the existing Ice Lance priority 21. Native casting owns
cooldown failure/fallback and spell_mage_deep_freeze's immune-target damage.
No pet, new AoE, movement, pull or lifecycle changes. See the matching core's
research/FROST_PROC_PACKET.md. Two policy tests added. Linux worldserver and
tests-common built successfully; its 184-case suite completed with 183 passed
and the existing expected failure. Windows worldserver/tests-common also built,
and all 184 CTest cases passed. Core/module diff checks passed.
Native proc consumption and immune-target behavior remain runtime-unverified.

## Fire proc and native override support - 2026-10-01, development

Adapt donor FireMageStrategy hot streak -> Pyroblast (25) and improved scorch
-> Scorch (19) at `7bae1b5c58c76a0aa20381155edc08096d1485b2`. Native Hot Streak
48108 overrides learned base 11366 to 92315; shared TryCast authorizes the base
then follows Unit::GetCastSpellInfo and its native cost flags before ordinary
cast validation. No forced replacement, GCD waiver or proc removal. Scorch
requires an active Critical Mass applier and matching loaded native proc metadata;
missing/mismatched metadata fails closed. Do not infer proc correctness from raw
DBC masks or policy tests. Living Bomb is deferred because its expiry explosion
needs AoE safety. See research/FIRE_PROC_OVERRIDE_PACKET.md in the matching core.
Windows/Linux built worldserver/tests-common with the final proc-metadata check.
Windows passed all 182 CTest cases; Linux completed its 182-case suite
successfully (181 passed plus the existing expected failure). Native behavior
and loaded Scorch proc-data correctness remain unverified.

## Fire/Arcane starter routes and Arcane actions - 2026-10-01, development

Adapted donor FireMageStrategy.cpp / ArcaneMageStrategy.cpp at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: named `fire` and `arcane` siblings,
donor starter default order, Arcane Blast/Missiles/Barrage, and stack-cap Missiles
priority 15. Cata Missiles requires native aura 79808; read CasterAuraSpell and
Arcane Blast 36032's cap from SpellInfo. Existing single-target threat and
cast/channel validators remain; native casting owns mana/stacks/procs. Shared
native spec refresh replaces the old Mage/Frost split and preserves the generic
unassigned route. Fire DoTs/Hot Streak, mana phases, AoE and full movement parity
are deferred. Hot Streak specifically needs native action-bar override resolution;
do not force its replacement spell. See core research/MAGE_SPEC_ACTION_PACKET.md.
Windows/Linux built worldserver/tests-common. Windows passed all 180 CTest
cases; Linux completed its 180-case suite successfully (179 passed plus the
existing expected failure). Native Arcane behavior is unverified.

## Native Cata Warrior additions - 2026-10-01, development

Add Colossus Smash 86346 to Arms/Fury and Enrage-gated Raging Blow 85288 to
Fury through the existing donor strategy/action surfaces. These are explicit
Cata adaptations, not unchanged donor actions. Priorities 28/26 place them
below stance support and around the existing primary/proc attacks. Own Colossus
Smash aura prevents clipping; native SpellInfo/HasAuraState supplies Raging
Blow eligibility. Native spell_warr_sudden_death owns the Colossus cooldown
reset, replacing the donor Wrath Execute-proc assumption. No manual reset,
armor/weapon-hit duplication or above-20% Execute route. Source-checked against
matching Cata scripts/DBC and donor `7bae1b5c58c76a0aa20381155edc08096d1485b2`.
See core research/WARRIOR_CATA_ACTION_PACKET.md. Windows and Linux built
worldserver/tests-common. Windows passed all 178 CTest cases; Linux completed
its 178-case suite successfully (177 passed plus the existing expected failure).
Native behavior remains unverified.

## Warrior proc response - 2026-10-01, development

Adapted donor ArmsWarriorStrategy.cpp / FuryWarriorStrategy.cpp and trigger
registrations at `7bae1b5c58c76a0aa20381155edc08096d1485b2`. Arms responds to
target-bound native dodge reactions or affecting Taste for Blood (60503) with
Overpower (7384) at donor relevance 24. Fury responds to Bloodsurge (46916)
with Slam (1464) at relevance 25, checking the native casting-time operation,
spell mask and full reduction. No unprocced Slam filler or forced instant cast.
Existing controlled melee actions repeat eligibility at execution; native
casting owns proc consumption, damage, rage and final calculated cast time.
See matching core research/WARRIOR_PROC_PACKET.md. Windows and Linux built
worldserver/tests-common. Windows passed all 176 CTest cases; Linux completed
its 176-case suite successfully (175 passed plus the existing expected failure).
Native proc behavior is not client-confirmed.

## Arms and Fury starter strategies - 2026-10-01, development

Adapted donor ArmsWarriorStrategy.cpp / FuryWarriorStrategy.cpp and native
spec selection at `7bae1b5c58c76a0aa20381155edc08096d1485b2`. Add sibling
`arms` and `fury` strategies: Mortal Strike, Bloodthirst, sub-20% Execute and
the corresponding native Battle/Berserker stance. Preserve donor relevance;
Strike remains the starter fallback. Shared native-primary-tree routing now
replaces the session's old generic-DPS/tank split via existing spec refresh.
Support validators take an explicit required tree; Protection defenses remain
Protection-only. Native costs, damage, healing, cooldowns, stance and rage
consumption are unchanged. No AoE, proc-heavy rotation or full spec parity is
claimed. See matching core research/WARRIOR_DPS_SPEC_PACKET.md.
Windows and Linux built worldserver/tests-common. Windows passed all 174 CTest
cases; Linux completed its 174-case suite successfully (173 passed plus the
existing expected failure). Native behavior is not client-confirmed.

## Protection stance and defensive support - 2026-10-01, development

Adapted donor TankWarriorStrategy.cpp, WarriorActions.h and HealthTriggers.h at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: Defensive Stance, Shield Block,
Shield Wall below 45% health and Last Stand below 25%, preserving donor
priorities and strict health boundaries. Existing Warrior action/trigger classes
now have an explicit self-target mode; hostile actions retain their validators.
Self support stays inside an existing routed Protection fight, without requiring
facing/melee reach. Native casting owns stance, shield, cost, proc and cooldown
requirements. No idle stance manager or emergency tick outside existing combat
routing. Health triggers use Warrior-specific names to avoid the rest trigger
collision. See core research/WARRIOR_DEFENSIVE_PACKET.md.
Windows and Linux built worldserver/tests-common with the final triggered-aura
correction. Windows passed all 173 CTest cases; Linux's 173-case suite completed
successfully (172 passed plus the existing expected failure). Native behavior
is not client-confirmed.

## Warrior tank rotation slice - 2026-10-01, development

Adapted donor TankWarriorStrategy.cpp / WarriorActions.h/.cpp at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: Devastate filler, Revenge,
Sunder Armor refresh/fallback and Sword and Board -> Shield Slam. Cata native
SpellInfo supplies the three-stack Sunder debuff (58567) cap and Revenge reactive state;
native scripts own Sunder application and Shield Slam cooldown reset.
Existing Protection strategy, default-off combat flag and controlled melee
target boundary remain. Shield Slam default 5.4 precedes donor Devastate 5.3;
explicit Sunder candidate replaces the donor ActionNode alternative. See the
matching core `doc/local/playerbots/research/WARRIOR_TANK_ROTATION_PACKET.md`.
Windows and Linux worldserver/tests-common built. Windows CTest passed all 172
registered tests; Linux's 172-case suite completed successfully (171 passed and
the existing expected-failure case). Native tank behavior remains untested.

## Warrior fallback correction from bundled test - 2026-10-01, development

The bundled outdoor session recorded Mage Fireball/Fire Blast/Molten Armor,
Priest Smite/Renew/Heal, Warrior Strike and native corpse opening by all four
bots. It also recorded repeated distance-leash disengages. The player reported
Warriors failing to resume on a later pull; logs showed their initial engagement,
then leash disengagement. Both saved Warrior primary talent trees were unassigned.
The initial fallback port admitted only Protection Warriors, leaving this role
dependent on selected-target assist while casters could resume independently.

All enabled Warriors now use the named fallback: Protection uses tank target,
unassigned/Arms/Fury use the donor general DPS selector. Mage/Priest routing is
unchanged. Melee DPS ranking uses native melee reach rather than the caster
range band. A pure routing regression checks every implemented route, disabled
gates and unsupported classes. Windows and Linux worldserver/tests-common each
built and passed all 171 tests after the explicit native enum include correction.
This does not fix or
relax the distance leash, switch active fights or qualify runtime recovery.
The completed test used the original binary copy; correction is not live-tested.

## Named combat targets and Protection tank fallback - 2026-10-01, development

Adapted active TankTargetValue smart ranking from donor
7bae1b5c58c76a0aa20381155edc08096d1485b2: lost aggro, owned melee, owned distant;
distance ties for lost aggro and lower own threat otherwise. Only Protection
Warriors qualify. Icon priority recognizes current non-tank roles without
guessing unported roles or another tank bot's rti. Explicit-main-tank preference,
taunts, threat writes and the inactive old selector are not ported.

Shared `dps target` / `tank target` return fresh GUIDs through a borrowed const
engine accessor, bound/cleared by Engine lifetime. Session ownership destroys
engine before context/AI. Idle Protection fallback requires EngineWarriorCombat;
Mage/Priest keep their gates. Selected targets, explicit commands and active
attacks retain precedence. No new core seam, SQL or retained Unit pointers.
Three pure tests were added; Windows and Linux worldserver/tests-common each
built and passed all 170 tests, including the final role-filter recheck. See matching
core research/TANK_TARGET_PACKET.md for native verification gaps.

## Controlled-party DPS fallback - 2026-10-01, development

Adapted TargetValue exclusion/priority gathering, DpsTargetValue caster/general
ranking and RtiValue icon defaults from donor
7bae1b5c58c76a0aa20381155edc08096d1485b2. Strategy exclusion hooks were already
imported; Engine now gathers fresh typed exclusions from active providers.
Shared manual `prioritized targets`, `rti` (skull) and `rti cc` (moon) values
register in current class contexts. No new command, persistence or addon claim.

The native map adapter returns a GUID, applying exclusions, CC markers and
native validity/leash checks only to already-controller-engaged creatures.
Explicit raid icon precedes donor caster/general ranking. Unknown DPS uses
general health/distance ordering instead of unsafe division. Living-near-member
count and the existing 25-yard eligibility are conservative Cata boundaries;
the 30-yard ranking boundary is not the donor configurable default (33.5).

Mage/Priest auto-assist uses this only when idle with no valid selected combat
target, under existing default-off engine gates. Explicit commands and valid
selected targets retain precedence; active fights and Warrior selection are
unchanged. No retained Unit pointers, second target owner or SQL. The matching
core adds only a bounds-checked read-only Group::GetTargetIcon accessor; native
marker writes and persistence are unchanged.
This is not complete named DPS target value parity, dynamic switching or
autonomous target permission. Encounter-specific exclusion providers remain ahead.

Four pure ranking tests and one active Engine exclusion test were added.
Windows and Linux worldserver/tests-common built and each passed all 167 tests;
native selection/gameplay remains bundled.
See the matching core's research/DPS_TARGET_PACKET.md for adaptations and gaps.

## Engaged attackers and healer balance - 2026-10-01, development

Adapted AttackersValue, BalancePercentValue in AttackerCountValues and
HealerShouldAttackTrigger from donor 7bae1b5c58c76a0aa20381155edc08096d1485b2.
Shared `attackers` reads native PvE threatened-by-me references for self and
nearby living same-map group members, retains deduplicated GUIDs and applies
native target/claim checks. It never changes the controlled target. Priority/
skull/duel/arena candidates and independent pet contributors are not ported.

Shared `balance` keeps donor creature-rank weights, roster denominator,
ten-member cap and 0–200 ratio. The numerator uses only living in-world
same-map members, not cross-map lookup. Both values recompute on Get rather
than reusing the donor one-second attacker cache; no retained Unit pointers.
Priest scheduled damage now uses donor-default balance mana thresholds
85/65/40, preserving existing 90% healing priority and controller/command gates.
No solo bypass, new settings, SQL, native threat mutation or core seam.

Three pure policy regressions were added; Windows and Linux worldserver/
tests-common built and each passed all 162 tests.
Native traversal/filtering is source-reviewed, not gameplay-tested. This is a
level/rank heuristic, not full target selection or measured encounter balance.
See the matching core's research/COMBAT_BALANCE_PACKET.md for adaptations/gaps.

## Single-target caster threat - 2026-10-01, development

Adapted ThreatValues / ThreatStrategy from donor
7bae1b5c58c76a0aa20381155edc08096d1485b2. Shared `threat` and reset-on-read
`neglect threat` values use native read-only current-target threat. Tanks are
living same-map group Protection Warriors, including humans; other tank roles
are not guessed. Without a recognized tank, damage remains allowed.
The Mage/Priest scheduled engines veto Single damage at the donor 80% threshold;
healing, buffs, cures and interrupts remain unaffected. Existing gates stay
default-off. No core seam, thread, SQL or new configuration.

The ratio explicitly guards zero division, saturates before uint8 conversion
and defers on non-finite input, rather than retaining donor arithmetic hazards.
Startup/fleeing and the one-shot bypass retain donor policy. AoE qualifiers,
FocusStrategy, Warrior auto-attacks and direct ExecuteAction interception are
not ported. No complete tank/threat-management parity is claimed.

Four pure policy/value tests and one Engine scheduling regression were added.
Windows and Linux worldserver/tests-common built and each passed all 159 tests.
Native reads/wiring are source-reviewed, not gameplay-tested.
See the matching core's research/THREAT_ENGINE_PACKET.md for scope and follow-up.

## Shared combat estimate values - 2026-10-01, development

Ported EstimatedGroupDpsValue / the current-target subset of EstimatedLifetimeValue
and mixed-gear helpers from donor 7bae1b5c58c76a0aa20381155edc08096d1485b2.
Named values register in all three implemented class contexts. The existing
map thread reads native alive/same-map server-origin group bots, carried usable
gear and current role. Human players are excluded. Float-only DPS caching keeps
the donor 20-second interval; group identity/map/bot-level changes invalidate it.
The borrowed AiObjectContext accessor binds/clears with existing context ownership.
No retained Player/Item pointers, equipment mutations, thread or native core seam.

Donor level/gear curves, role weights, quality scaling, best-slot/top-twelve gear
aggregation and party/raid bonuses are retained. Cata robe and holdable mappings
are included. Only supported bot roles/levels 1–80 are modelled; 81–85 is unavailable,
not extrapolated. Unknown estimates return zero; the single-target lifetime value
does not implement arbitrary target qualifiers or the donor multi-attacker penalty.
Shadow Word: Pain now requires its donor eight-second health/DPS gate at execution;
unknown estimates skip the DoT while leaving other spells available. This is an
estimate, not measured damage or complete Cata role/profile parity.

Seven new pure tests cover curves/gear/roles/group bonuses/slots/lifetime policy.
Windows and Linux worldserver/tests-common built and each passed all 154 tests.
Native traversal, filtering and cache/context
ownership are source-reviewed; accuracy and gameplay remain bundled checks.

## Priest healer damage actions - 2026-10-01, development

Extended the same donor PriestHealerDpsStrategy at
7bae1b5c58c76a0aa20381155edc08096d1485b2 with Shadow Word: Pain (5.5), Holy
Fire (5.4), existing Smite (5.3) and Mind Blast (5.2). One spell table supplies
strategy wiring and action registration; each action rechecks its own learned
spell and the existing healing/mana/target gates. The common trigger is no
longer dependent on knowing Smite. No session, movement, core or SQL change.

Native Cata DBC rows confirm Priest class mask 16 for 589 (level 4, periodic
damage), 14914 (level 18, direct/periodic damage) and 8092 (level 9, direct damage).
Periodic actions check only their caster-owned native aura and do not reapply
while it remains. This follows donor owner-aware aura checks; no manual effects.
Holy Fire's donor minimum lifetime is zero. Shadow Word: Pain's eight-second
estimated-lifetime gate requires an unported group-DPS value and is explicitly
deferred; short-lived-target efficiency is not equivalent to the donor.

The wiring regression now checks all four donor priorities; two new tests check
the Cata identities and periodic-aura policy. Windows and Linux built
worldserver/tests-common and each passed all 147 tests. Gameplay remains part
of the existing bundled check. No wand shooting,
AoE, Shadow rotation, balance values or estimated-lifetime substitute added.

## Priest healer DPS - 2026-10-01, development

Adapted PriestHealerDpsStrategy / HealerShouldAttackTrigger from donor
7bae1b5c58c76a0aa20381155edc08096d1485b2. The `healer dps` strategy wires
`healer should attack` to real Smite at 5.3, below heal/cure priorities. Native
Cata DBC confirms spell 585, level 1, Priest mask 16 and school-damage effect 2.
The existing EnginePriestHeal gate remains default-off; no learning or SQL seam.

The map adapter's existing GUID target and assist flag own stationary Priest
support, without melee Attack or MoveChase. Commands/target invalidation precede
the same 750-ms healing tick; ending support restores idle formation follow.
Damage requires the controller fighting a native-valid nearby creature,
eligible healing candidates at >=90% health and mana >=85%. Trigger/action both
recheck. Donor balance is not ported: this explicitly uses its conservative mana
branch, without the solo bypass. Explicit attack cannot start a Priest-only pull.

Four new tests cover donor wiring, eligibility/mana, healing/target changes and
engine-queued damage yielding to changed eligibility. Windows and Linux built
and each passed 145 tests. Control/movement is source-reviewed;
damage, healing responsiveness and follow resumption remain for a bundled live
check. No own-cast cancellation, full Shadow rotation, AoE or balance parity.

## Party-cure healthy-target correction - 2026-10-01, development

Source review for the next healer-DPS slice caught a Cata adaptation error in
the disease port below: its reused TryInHealthOrder erased candidates at 90
percent health or above. Healthy players can still have dispellable diseases.
Ordering is now a separate TryInPriorityOrder helper; healing keeps its original
health filter, while cure target detection/execution use the unfiltered order.
Native life/map/range/LOS/disease checks remain unchanged.

Three added regressions retain healthy/full-health members, preserve stable
health ties and rejected-cast fallback, and handle empty cure lists. Existing
healing regressions still enforce the old healing-only cutoff. Windows and Linux
built and each passed all 141 tests. This fixes selection,
not proof of native aura removal. The next healer-DPS packet is mapped but not
implemented; Priest offensive target/control ownership must be completed first.

## Priest disease cure - 2026-10-01, development

Ports the disease subset of donor PriestCureStrategy, CureDiseaseTrigger and
PartyMemberCureDiseaseTrigger plus CurePriestStrategyActionNodeFactory's real
Cure fallback at 7bae1b5c58c76a0aa20381155edc08096d1485b2. Cata DBC inspection
confirmed Cure Disease 528, class mask 16, level 22, effect 38/dispel type 3;
Wrath Abolish Disease 552 is absent. The `cure` strategy therefore wires
`cure disease` / `party member cure disease` directly to `cure disease` /
`cure disease on party`, preserving donor priorities 31/30 without a dummy
Abolish creator.

The existing Priest engine/candidate helper owns self/controller/group targeting.
Party cures exclude self and try eligible members in health order; a native
cast rejection on one member does not starve the others. Triggers and actions
ask native GetDispellableAuraList for disease eligibility, preserving polarity,
passive/zero-charge/100-percent-resistance rejection and Unholy Blight protection.
Aura pointers stay inside the current call; native TryCast/EffectDispel owns
actual aura removal, resistance and resource effects.

Existing EnginePriestHeal gates execution. Cure defers during rest or queued/
pursued loot. Critical healing priorities remain higher. Magic/poison/curse
dispels, enemy purge, talent dispel changes and AoE cure are not included.
No new config key, scheduler, core seam or database change is added.
Three pure tests cover names/priorities and enabled/alive/learned/native-allow
policy. Windows and Linux worldserver/tests-common builds each passed all 138
tests; native disease removal and protection/
resistance behavior remain unverified in game.

## Heroic Strike and conditional Frost Ice Lance - 2026-10-01, development

Adapts donor FuryWarriorStrategy/TankWarriorStrategy rage triggers,
GenericTriggers::RageAvailable, FrostMageStrategy's Ice Lance default and
CastIceLanceAction at 7bae1b5c58c76a0aa20381155edc08096d1485b2. Generic Warrior
uses `medium rage available` (40 displayed rage), ACTION_DEFAULT + 0.1;
Protection uses `high rage available` (60 rage), ACTION_HIGH. Native Cata power
stores rage in tenths, so reserves are 400/600. The action rechecks the current
primary tree and learned spell before using existing native TryCast.

Read-only Cata DBC checks confirmed Heroic Strike 78, class mask 1, level 14;
Ice Lance 30455, mask 128, level 28. Fingers of Frost proc 44544 applies aura
type 262. Frost Mage uses the native Mage aura-state override and frozen-target
checks from Unit.cpp's Ice Lance damage path. Native casting/damage owns proc
consumption, rage spending and effects; none are set manually.

Frost retains Ice Lance's default slot at 5.3 and adds frozen/proc priority 21.
The action requires current frozen/proc eligibility, rejecting expired states.
This conditional usefulness/priority is an explicit Cata adaptation: donor
Wrath's Fingers of Frost trigger favors Deep Freeze/Frostbolt, and its default
Ice Lance permits movement filling. Unrestricted movement filling, Deep Freeze,
pet/proc rotations and Generic Mage changes are not part of this port.

Both actions use existing default-off combat-engine gates and existing class
contexts. No AoE scan, spell grant, talents, config/database/core changes or
new scheduler. Three pure regressions cover rage boundaries/reserves, known-spell
gates and frozen/proc conditions. Windows and Linux worldserver/tests-common
builds each passed all 135 tests; live resource
spending, proc consumption and damage remain unverified.

## Current-target interrupts - 2026-10-01, development

Adapts donor InterruptSpellTrigger, Warrior Pummel and Mage Counterspell
actions at 7bae1b5c58c76a0aa20381155edc08096d1485b2. Implemented Warrior and
Mage combat strategies register `pummel` / `counterspell` at ACTION_INTERRUPT
(40), under their existing default-off combat-engine flags. Both use the
already-selected GUID-based combat target; no enemy-healer scan is added.

Read-only Cata DBC checks confirmed Pummel 6552, learn level 38, class mask 1;
Counterspell 2139, learn level 9, class mask 128. Both use native interrupt
effect 68. Only learned spells are attempted. Trigger and action revalidate
the same preparing-with-cast-time/channeling and native CanBeInterrupted policy
used by Spell::EffectInterruptCast. Native TryCast still owns range/LOS,
cooldowns, resources, stance, GCD and spell execution. The module never calls
InterruptSpell or locks a spell school manually.

The Mage does not cancel its own current cast to interrupt. Cross-bot
coordination, Priest Silence, healer targeting and movement-to-interrupt remain
follow-ups. These are bounded current-target actions, not full donor parity.
Three pure tests cover cast-state/native-allow policy and trigger names/priority.
Windows and Linux worldserver/tests-common built and each passed all 132 tests
after refreshing build definitions for new files. Landed interrupts remain unverified.

## Active-spec combat strategy refresh - 2026-10-01, development

Adapts donor AiFactory::AddDefaultCombatStrategies and
PlayerbotAI::SelectiveResetStrategies at
7bae1b5c58c76a0aa20381155edc08096d1485b2. The existing single-engine map adapter
now reselects implemented combat routes from native active primary talent tree
before ticking: Protection Warrior -> tank, other Warrior -> warrior; Frost
Mage -> frost, other Mage -> mage. Priest retains the explicit heal fallback.
This fixes login-only selection, not missing spec rotations or talent spending.

Mage combat siblings now have their own context, matching Warrior, so switching
does not remove shared buff/rest/loot/armor strategies. Unchanged routes preserve
the queue. Changed routes use existing AddStrategy/Init to remove the old sibling
and clear obsolete queued actions. Current native casts are not cancelled.
No new scheduler, core hook, database or config setting is introduced.

Three pure regressions cover Cata-tree/fallback mapping, sibling/shared strategy
preservation, and unchanged-queue versus changed-route reset. Windows and Linux
worldserver/tests-common builds each passed all 129 tests. Live spec transitions remain
unverified. See the matching core's SPEC_STRATEGY_REFRESH_PACKET.md for scope.

## Bounded corpse collection and movement - 2026-10-01, development

Follow-up to the opening slice below, adapting donor LootObjectStack,
LootNonCombatStrategy and MovementActions.cpp::MoveToLootAction at
7bae1b5c58c76a0aa20381155edc08096d1485b2. The `far from loot target` trigger
and `move to loot` action keep donor relevance 7. Discovery is limited to
observed defeated targets and eligible controller selection; an eight-entry
map-owned GUID collection expires entries after 60 seconds and selects the
nearest currently eligible corpse. It retains no donor Player pointer.

Native pathfinding handles short detours only: corpse within 15 yards of the
bot and 20 yards of the controller. The existing session movement owner pauses
follow, cancels for combat/control/transfer/disable/ineligibility, and restores
formation on completion. One GUID/timestamp bounds pursuit to ten seconds;
timeout uses the existing per-corpse backoff. Rest/armor defer while pursuing.
No new scheduler, broad area search, teleport, database or config key is added.
The opening mailbox and native permissions/storage remain unchanged.

Three pure regressions cover collection capacity/deduplication, expiry/wrap and
single-pursuit timeout/reuse; strategy coverage includes both donor stages.
Final Windows and Linux worldserver/tests-common builds passed all 126 tests
on each platform, including the movement-control guard. Live detour/path-cancellation behavior remains
unverified and belongs in the next bundled recovery/loot check.

## Nearby corpse opening/storage - 2026-10-01, development

The next bounded slice adapts LootNonCombatStrategy/OpenLootAction/StoreLootAction
at donor 7bae1b5c58c76a0aa20381155edc08096d1485b2. It retains `loot`, `can loot`
and `open loot` names and opening relevance 8. The initial strategy deliberately
omits distant-loot movement, area discovery, gathering and skinning instead of
registering unimplemented actions. It considers the last defeated combat target,
or the controller's selected corpse if that target is not nearby. Selection is
cached for two seconds and considers at most two identities, not a nearby scan.

Playerbots.Loot.Corpses.Enabled defaults off and is cached on load/reload. Actors
must be alive, out of combat/casting/transfer, unmounted, near a live controller,
and not actively resting. Candidates must be lootable, within native interaction
distance/LOS, and assigned to the bot or its group. A bounded eight-entry attempt
history supplies a 30-second per-corpse backoff, including inventory failures;
this is not the donor's full cached item-usage/loot-strategy policy. All native
allowed/owner item slots and coins are attempted; currencies are deferred.

One GUID-only, generation-checked mailbox bridges map-thread engine decisions
to the existing WorldSession unsafe/world update. It is not another scheduler.
The world consumer rechecks current follow-controller identity, full-control
authorization, feature gate, actor/corpse eligibility and absence of an unrelated
active loot window. No raw player, corpse, loot or packet pointer survives a tick.
Explicit movement/combat requests, death, transfer or disable cancel pending
work; stale completion cannot clear a newer request. Follow pause/resume uses
an explicit receipt flag, including completion before the next map update.

The required native core seam adds an optional typed LootResponse output to
Player::SendLoot. Ordinary callers keep the default null path. A provided result
is reset at entry and filled only by the normal permission-checked loot response
builder; generation, ownership, roll creation and looter registration remain
native. This avoids reading discarded socketless outbound packets or duplicating
permission logic. The world consumer uses ordinary native money/autostore
handlers and release, preserving HandleLootOpcode's post-open cast/aura
interrupt effects. Only ALLOW_LOOT/OWNER slots are attempted; roll, locked and
master slots are skipped. No manual money restoration, item awards, loot-GUID
overwrite, Wrath response decoding or synchronous module SQL is used.

The action's true result means request accepted. PB-LOOT logging means native
opening accepted, not storage success; inventory failures and awards remain
native. Four pure tests cover strategy wiring, bounded mailbox lifecycle,
cancellation/stale completion, follow-resume signaling and backoff/wrap handling.
Native storage, sharing/rolls, full bags and rest/follow timing still
require further bundled evidence; earlier 119-test evidence is historical.
Windows and Linux worldserver/tests-common builds each passed all 123 tests
after the final interruption guard and test-only GUID link/constructor
corrections. Linux's completed larger rebuild was followed by an incremental
current-source check and another full test pass.

The 2026-10-01 outdoor mixed-party check recorded native corpse opening by all
four bots and 17 accepted Mage offensive casts. The party returned to follow
after kills, and bot save/logout and server shutdown completed cleanly. Opening
logs do not prove item awards or roll resolution. No accepted Priest healing
casts were recorded. Food/water had not been provisioned and the level-20 Mage
had no armor spell, so rest and armor were not exercised. The reused Warriors
were level one; the recovery harness now requests native offline level
normalization before admission. That fixture correction is not runtime-verified.

## Native companion group-loot preference - 2026-10-01, development

Loot source review refreshed upstream master at
7bae1b5c58c76a0aa20381155edc08096d1485b2 and inspected LootNonCombatStrategy,
AvailableLootValue/HasAvailableLootValue, LootObjectStack, LootAction and
LootRollAction. Full corpse looting and item-usage-based rolling are not ported.
In particular, donor StoreLootAction depends on an outbound loot response that
server-origin Cata sessions currently discard. No Wrath response layout or
unsafe map-thread Group vote calls were copied.

The first implementation reuses Cata's native Player pass-on-group-loot preference
behind Playerbots.Loot.PassOnGroupLoot (default off, cached on load/reload).
Session-owned state captures/restores the prior preference on enable/disable;
it holds no Player/Group/Roll pointers. It applies to new rolls, not existing
pending rolls, and does not collect money/items or evaluate equipment upgrades.
Ordinary players and other loot methods are not configured by this policy.

A matching native core correction is required: GroupLoot must count its initial
auto-pass votes exactly once. The follow-loot-rules quest branches in GroupLoot
and NeedBeforeGreed must also honor/count the native preference. This corrects
ordinary native opt-out as well, independently of the module flag. Reward
distance, eligibility, vote resolution and awards remain native. The older
published core pin lacks this correction; use matching development sources.

Four pure policy tests cover disabled noninterference, snapshot/restore,
reassertion and reenable. Native roll creation/resolution and all-pass timeout
behavior remain bundled runtime checks, not proven by these policy tests.
Windows and Linux worldserver/tests-common builds each passed all 119 tests
after the final core correction; whitespace checks passed in both repositories.

## Mage self-armor strategies - 2026-10-01, development

Adapts GenericMageNonCombatStrategy's MageBuffManaStrategy/MageBuffDpsStrategy
and MageArmorTrigger/MoltenArmorTrigger at donor
7bae1b5c58c76a0aa20381155edc08096d1485b2. The `bmana`, `bdps`, `mage armor`
and `molten armor` registry names and relevance 19 are retained. Both strategies
are registered; mutually exclusive triggers select from the current active
talent tree and learned spells on each evaluation, avoiding a stale login-time
armor choice. Arcane prefers Mage Armor; other/unspecialized bots prefer Molten
Armor, then learned Mage Armor, then Frost Armor. This is a bounded Cata policy,
not a port of full donor profiles or a claim of optimal endgame rotations.

Cata's unranked spell identities are Mage Armor 6117, Molten Armor 30482 and
Frost Armor 7302. The existing caster DBC checker covers these identities and
learn levels; Wrath's separate Ice Armor fallback is not copied. Unlike the
donor's any-armor guard, a changed spec may replace an existing different armor.
The native cast/exclusivity path owns replacement; no old aura is removed before
a successful cast. The action rechecks eligibility and uses ordinary cooldown,
GCD, mana and learned-spell validation. Combat, transfer, mounting/flight,
missing/dead/distant controller and active carried-food recovery suppress it.

Playerbots.Mage.Armor.Enabled defaults off and is cached at configuration
load/reload, with invalid module configuration forcing it off. It shares the
existing two-second noncombat engine cadence with rest, without adding a timer
or scheduler. Windows and Linux worldserver/tests-common builds each passed all
115 automated tests. The read-only local Cata DBC check also passed, confirming
Molten Armor at level 34, Frost Armor at 54 and Mage Armor at 68. Native armor
execution and spec-change replacement belong to the next bundled party check.

## Shared carried food/drink recovery - 2026-10-01, development

The next unpublished batch adapts UseFoodStrategy, scalar health/mana triggers,
and the inventory/usefulness relationship from UseItemAction at donor
7bae1b5c58c76a0aa20381155edc08096d1485b2. The donor `food`, `drink`, `low health`
and `low mana` registry names and relevance 3 are retained. The initial Cata
thresholds are 40% health and 20% mana; no cheat branch or inventory replenishment
is included. Playerbots.Rest.Enabled defaults off and is cached at configuration
load/reload, with invalid module configuration forcing it off.

Warrior/Mage/Priest contexts register the shared strategy. A two-second cached
inventory candidate stores only an item GUID; execution resolves it again and
checks that it remains carried, usable and eligible. Candidate selection reads
Cata's first valid on-use ItemTemplate effect, native food/drink categories and
matching regeneration auras. This first batch excludes combined-category feasts,
purchases, conjuring, potions and bank items. It does not infer regeneration from
Wrath category numbers alone.

Cata SpellCastRequest/PendingSpellCastRequest and native RequestSpellCast preserve
ProcessItemCast validation, binding, item-use hooks, cast-item consumption and
cooldowns. No Wrath wire layout, manual resource restoration or manual item
deletion is used. Requests are submitted only outside casting/GCD so no delayed
rest request is intentionally queued. A rest spell identity tracks the movement
pause; aura completion/failure, controller combat/range, death, transfer, module
disable and explicit movement/combat requests release it. Existing follow identity
is preserved; resume uses current formation/path logic. Only the recorded aura is
removed by rest cleanup. Native aura application supplies seated state.

The existing engine remains the decision owner. While rest is enabled, the map
adapter prevents duplicate engine decisions in the same update. Existing class
action gates prevent the new rest cadence from enabling unrelated combat/healing
routes. Priest healing and resurrection remain above rest relevance. Four new
pure tests cover strategy wiring, resource thresholds and eligibility, not native
item use or actual follow timing. Windows worldserver/tests-common built and all
113 tests passed. Linux worldserver/tests-common also built and all 113 tests
passed after the final cooldown/logging review. Live
execution remains for the next bundled mixed-party check. Earlier milestone
evidence below applies to the published snapshot, not this new batch.

## Linux build acceptance - 2026-10-01

Ubuntu 22.04, GCC 11.4, CMake 3.22 and Ninja built worldserver, authserver and
tests-common with Playerbots and AHBot enabled, Release configuration and both
core/script precompiled headers disabled. CTest passed all 109 tests. This run
required explicit array and map-store headers previously hidden by incidental
includes. Revision stamping was disabled for the mounted Windows worktree;
that build arrangement is not a change to runtime or source revision policy.
This verifies Linux compilation and headless tests, not a Linux realm/database
playtest, every toolchain, or full donor gameplay parity.

## Explicit map-store dependency - 2026-10-01

The GCC 11 build with core precompiled headers disabled exposed an undeclared
sMapStore and incomplete MapEntry in PlayerbotSessionBehavior's instance-entry
path. That source now includes DBCStores.h directly, which declares the store
and includes the native map-entry definition. No routing or gameplay behavior
changes. The subsequent incremental Linux build and all 109 tests passed.

## Ordinary-player managed lifecycle check - 2026-10-01

An ordinary client account created and logged into a Blood Elf Warrior. Native
character storage and the auth realm index confirmed the character and count.
With an explicit directional account link, the player discovered the configured
bot under MultiBot's My Bots roster and used its buttons to connect, disconnect
and reconnect. The harness observed native character online transitions and
cleared bot-account online state after disconnect; server logs recorded repeated
login/logout completion. The bot also accepted a party invitation and followed.
Human logout, explicit final bot cleanup and world/database shutdown completed
cleanly. No GM override was used.

An earlier fixture lost its processes without clean-shutdown evidence; its cause
remains unknown. Database recovery preserved the client-created character and
confirmed accounting before the corrected check reused it. The successful rerun
does not diagnose that stop. Harness password/name setup errors were corrected.
This check does not prove autonomous admission, factory player endpoints, every
permission-denial case, full donor UI compatibility or Linux build/runtime.

## Mage/Priest party buffs through the engine - 2026-09-30

Upstream master was rechecked and remains
7bae1b5c58c76a0aa20381155edc08096d1485b2. This adaptation follows
MageBuffStrategy in src/Ai/Class/Mage/Strategy/GenericMageNonCombatStrategy.cpp
and PriestBuffStrategy in src/Ai/Class/Priest/Strategy/PriestNonCombatStrategy.cpp,
plus their class buff triggers/actions. Registered `buff` strategies now select
`arcane intellect on party` and `power word: fortitude on party` actions through
the existing class engine. Wrath spell naming is retained for registry congruence;
Cata Brilliance/Fortitude spell and single/party aura IDs remain authoritative.
Priest buff relevance is reduced from donor 11 to 10 so existing light healing
and resurrection win. Divine Spirit, armor, consumables and wider buffs are absent.

The new default-off EnginePartyBuff gate selects this route instead of direct
Mage/Priest upkeep. Priest healing uses the same engine without a second buff
tick when its route is enabled. A per-trigger CalculatedValue caches only a
boolean for two seconds; actions recheck current targets, configuration and
native eligibility before casting. No Player pointer is retained across ticks,
no map-thread SQL or independent update loop is added. The existing direct
fallback uses the same aura definitions and candidate visitor, preserving its
target order and eligibility when the feature is off. Worldserver/tests-common
built with both modules enabled and all 109 automated tests passed. The three
new cases check strategy naming/handler priority and Cata aura definitions,
not native casts. The first link missed newly added files in the stale generated
source list; refreshing the existing CMake tree resolved it. Runtime confirmation
belongs to the next mixed-party check; this batch did not start a realm.

## Completed disposable factory batch - 2026-09-30

The corrected worldserver/authserver build passed all 106 automated tests.
The bundled CheckFactory runtime run then passed on a fresh disposable clone:
disabled mutations and absent schema rejected without aborting; missing dedication,
nonempty, privileged and online accounts rejected; enrollment confirmed the stored
record; native Mage creation returned a ready GUID after accounting completion.
An exact rerun repaired an intentionally stale realm count without creating or
changing the character. Conflicting intent rejected. Explicit managed admission
then completed login, save/logout and account-online cleanup, followed by clean
world/database shutdown. Only the clone received schema, account and config writes.

This supersedes the pending factory runtime statements in the earlier entries
below. The first failed run was a duplicate-key harness error; the second exposed
the fatal missing-table query corrected by metadata preflight. Ordinary client
creation, addon-managed lifecycle, broader gameplay, Debug runtime and Linux
build/runtime remain unverified by this server-only batch. No automatic account
creation, population, admission or player-control grants have been added.

## Optional-schema preflight correction - 2026-09-30

The missing-schema runtime case exposed a real server abort: MySQL error 1146
reaches TrinityCore's MySQLConnection::_HandleMySQLErrno fatal schema handler.
A missing optional table does not simply produce an empty query result. Factory
inspection and enrollment now check information_schema for the required column
names/types before referencing the module table. Missing/incompatible metadata
rejects the request without issuing that optional-table query. The check is not
cached, allowing an operator to apply the schema before a later command.

The core's fatal SQL error policy is unchanged. This preflight does not promise
to survive arbitrary concurrent schema changes or all database failures; apply
the documented schema before enabling mutations and do not alter it during
active requests. The previous runtime run proved creation/reuse/accounting but
stopped at a harness config error; the next run aborted at the missing-schema
case. Neither failed run established full managed admission success. The completed
preflight rebuild and fresh runtime rerun are recorded above.

## Factory runtime batch and typed SQL reads - 2026-09-30

The core's existing disposable lifecycle helper now has a bundled CheckFactory
scenario; no new service launcher is required by the module. It accepts an
explicit validated source checkout for a stopped seed, clones it, then uses the
current binaries and optional ownership schema only in that private clone.
Initial creation/reuse/accounting checks succeeded. The first run stopped at
managed admission because the harness generated duplicate Managed.Enabled INI
keys, causing native config reload to fail; it did not establish full admission
success. The harness now replaces the key and polls asynchronous factory status.

Field getters were also aligned with SQL metadata: account-online uses its native
byte getter, and aggregate/epoch/security/GUID numeric expressions are explicitly
CAST AS UNSIGNED with 64-bit getters. TrinityCore's TRINITY_DEBUG type checks
otherwise return zero for a mismatched getter; eligibility reads must not silently
permit an online/privileged account or ignore a decimal SUM. This is source-audited
Debug compatibility, not a completed Debug build/run. Corrected worldserver,
authserver and tests-common built and all 106 headless tests passed. A fresh
runtime rerun is pending; ordinary client creation/gameplay remain untested here.

## Explicit enrollment, native submission and reuse recovery - 2026-09-30

The next operator slice is now implemented in source under the same donor-shaped
RandomPlayerbotFactory owner. `Factory.Enabled` defaults off and requires valid
module configuration. Console-only `managed enroll`, `managed provision` and
`managed factory-status` use no addon/player endpoint. No credentials or native
account creation are introduced; accounts are pre-created through native admin.

Enrollment requires an empty ordinary account, no existing ownership record,
valid normalized intent, account expansion eligibility and successful native
account/character/name reads. It submits a plain auth INSERT...SELECT, rechecking
auth eligibility without replacing conflicting rows. World-thread callback
processing confirms the transaction and actual identity/intent row; a zero-row
successful transaction is not success. One evidence row per account is supported
in the initial optional schema. No schema migration or live writes were run.

Provisioning consumes current verified evidence/identity, then calls native typed
creation for an empty account. Exact reruns use World::BeginCharacterReconciliation:
native cache ownership and one matching database GUID are required, then a
dedicated world-owned context holds the account reservation through authoritative
realm-count reconciliation. It does not repeat character creation or save.
The core creates the receipt; the module only retains its const view. Status
reports native rejection, pending accounting, failed accounting with recovery
GUID, abandonment or confirmed readiness. Accepted requests are not completion.

Up to 16 account histories exist in process, with no overlapping pending attempt
on an account. Factory disable blocks new mutations but pending enrollment
callbacks continue. No auto-admission, roster/config mutation, player-control
grant, population, deletion or password change occurs. Enrollment is explicit
operator dedication, not a distributed ownership proof. It does not reserve
native sessions or certify cross-database atomicity; submission rechecks current
eligibility and obtains its own native reservation. Restart/rerun consults
persistent evidence and actual native identity rather than in-memory history.
See sql/README.md for commands and limitations. This slice built worldserver,
authserver and tests-common with both modules enabled and passed all 106 automated
tests, including the new bounded/overlap request-policy case. The preceding
reader slice passed 105 tests. Operator schema/queries/callback timing remain
runtime-unverified; pure policy tests do not certify native persistence.

## Persistent ownership reader and operator inspection - 2026-09-30

The module now provides an optional additive auth ownership schema and a native
read-only reader under the existing RandomPlayerbotFactory owner. This is Cata
adaptation around the donor's account selection, not a copied bulk creator.
Records bind an explicit operator dedication to native account ID/name/join epoch,
realm, evidence version and exact character intent. They contain no credentials.
No rows are inferred from name prefixes or config links. The schema is manual,
not registered in the core updater, so disabled/module-off builds need no table.

`Playerbots.Factory.InspectionEnabled` defaults off and also requires valid module
configuration. Console-only `server playerbotdev managed inspect <account ID>`
reads the record, validates account/profile eligibility and compares native
characters/name availability. Aggregate queries return a row even for zero
characters; missing results reject rather than silently meaning empty. SQL uses
numeric IDs and native escaping for the normalized name. Query failures never
permit creation or reuse. These are synchronous operator reads, not AI ticks.

The diagnostic never writes, enrolls, provisions, updates the roster or admits
bots. Create/reuse results remain drafts. Other-realm counts are auth-index
evidence, not verified remote character databases; administrator dedication and
native validation remain necessary. Native character limits/class unlocks and
submission-time race checks belong to the creation path. Enrollment and rerun
accounting recovery remain the next work. See sql/README.md for the schema and
validation boundary. No migration or account/character mutations were executed.
Two added headless cases cover evidence identity/version binding and missing
fields. This reader slice built worldserver/authserver/tests-common and passed
all 105 automated tests. Schema/query runtime validation remains unperformed.

## Provisioning accounting and ownership decisions - 2026-09-30

Creation-only contexts now keep their account reservation through a separate
Reconciling receipt phase. The original native realm-count transaction is awaited
before a corrective write, preventing the two writes from overtaking each other.
After native character success or rejection, the context reads authoritative
COUNT(guid) without GROUP BY (so zero characters still yields a result row), then
awaits a LOGIN_REP_REALM_CHARACTERS transaction. Missing queries, out-of-range
counts and failed corrective commits yield AccountingFailed, not ready success.
An original failed optimistic write can be repaired after the character result
is known. Ordinary client creation retains its existing transaction behavior.

The provisioning API returns a const receipt view. Ready GUID access stays zero
until character creation and accounting both succeed; a separate committed GUID
accessor retains evidence for recovery after accounting failure or abandonment.
This is not a cross-database atomic transaction or a distributed account lease.
Crash/restart reconciliation and a reuse-specific operator path remain unwired.

`src/Bot/Factory/PlayerbotFactoryOwnership.h` defines module-side creation/reuse
decisions: verified dedicated ownership and an eligible profile are required;
only an empty account may create, and only one exact existing account/name/race/
class/gender identity may reuse. Prefixes, conflicts and extra characters do not
grant ownership. Inputs must be verified native/persistent facts, not addon data.
The evidence reader/storage and operator flow are not implemented by these pure
policy helpers. No live provisioning or new account/character writes occurred.

Worldserver, authserver and tests-common built successfully with both modules
enabled; all 103 automated tests passed. Six added cases cover accounting
readiness/failure/abandonment and exact-identity ownership/reuse decisions.
These are headless policy/receipt checks, not native database timing evidence.

## World-owned native provisioning contexts - 2026-09-30

World::BeginCharacterProvisioning now owns an explicit CharacterCreation
WorldSession context, separate from clients and admitted server-origin bots.
It has no socket, character admission GUID or bot hooks, never enters m_sessions,
and skips account-online writes on destruction. It cannot be initialized or
admitted through normal session APIs. Native creation queries/transactions are
pumped on the world thread; the context remains owned until its receipt is
terminal. At most 16 contexts exist, with one account reservation each.

Existing sessions, duplicate reservations, shutdown, online account profiles,
privileged accounts in any realm and banned accounts reject new provisioning.
Human and bot admission cannot replace a pending context. The callback loop
snapshots account keys rather than holding unordered-map iterators through
scripts that could submit another request. World destruction abandons unfinished
receipts; it does not prove a submitted transaction was cancelled. KickAll does
not destroy an executing creation owner or prematurely release its reservation.

This is a trusted native service, not a player/addon/console endpoint. Its
caller must establish dedicated-account ownership and feature policy. No module
caller, account creation, configuration switch, identity adoption or live database
write was introduced in this batch. Factory ownership/reuse and realm-count
reconciliation remain prerequisites to exposing the default-off operator flow.
The account profile uses a prepared read of existing auth tables; no migration
or copied character-row writes are involved. Reservation/profile policy tests
do not validate native callback timing or persistence.
Worldserver, authserver and tests-common built successfully with both modules
enabled; all 97 automated tests passed, including the two new policy cases.
No runtime provisioning or database migration was performed.

## Typed native character creation and completion seam - 2026-09-30

Cata's HandleCharCreateOpcode now decodes the packet and calls
WorldSession::BeginCharacterCreation with a copied CharacterCreateInfo. The
existing validation/query/save body is retained in CreateCharacter; creation
rules, native Player cleanup, scripts, realm-count submission and character
cache ordering are not replaced by module SQL. CharacterCreateInfo has a typed
constructor matching the donor factory's input shape.

NativeCharacterCreationReceipt distinguishes pending, native rejection,
successful character commit/cache publication with GUID, and abandoned owner.
It is world-thread-only and contains no session/Player pointers. One request
cannot overwrite another pending request, and terminal outcomes do not change.
An abandoned transaction may still commit; this is not a rollback receipt or
an atomic character/login database result. The existing realm-count transaction
still has its separate native submission/completion semantics.

Active/loading players and existing server-origin bot sessions cannot begin
creation through this entry point. The factory does not call it yet: temporary
provisioning ownership, callback pumping, account policy, reuse/reconciliation
and the default-off operator endpoint remain the next slice. Do not construct
a fake client session or fabricate a create packet to bypass that work.
The ordinary client creation path now shares this seam and needs inclusion in
the next integrated disposable creation/lifecycle check. Receipt-only unit
checks do not prove a successful database transaction or normal client creation.
Combined worldserver/tests-common build passed with both modules enabled;
all 95 automated checks passed. A source-body comparison against the current
core HEAD confirmed that native creation validation/query/save logic differs
only in success GUID reporting. No live character creation was performed.

## Initial native factory draft - 2026-09-30

The new `src/Bot/Factory/RandomPlayerbotFactory.{h,cpp}` adapts the appearance
selection in upstream RandomPlayerbotFactory::CreateRandomBot at
`7bae1b5c58c76a0aa20381155edc08096d1485b2` (upstream master rechecked today).
It retains the donor factory name/placement and author notices, but deliberately
stops before Player/session allocation or database writes in this first slice.

Cata uses RaceID/SexID/BaseSection/VariationIndex/ColorIndex instead of the
donor CharSections field names. The adapter checks native race/class start data,
world expansion and disabled-creation masks, filters player/DK appearance flags,
matches face skin and hair/facial colors, and delegates final validity to
Player::ValidateAppearance with creation mode enabled. The no-facial-hair
exceptions match that native validator. Selection is deterministic for the
read-only draft, not a random-population implementation.

`PlayerbotAppearance.h` bounds the search and rejects missing/oversized data
instead of randomly indexing empty donor arrays. Five regression cases cover
missing data, field/color mapping, rejected alternatives, native-call bounds
and mismatched-color failure. The console-only `managed appearance` command
reports the draft without implying account/name/limit/DK-unlock eligibility.
No new runtime admission gate, creation endpoint or identity adoption is added.
The full provisioning path and Cata DBC runtime check remain pending.
Combined worldserver/tests-common build with both modules enabled passed;
all 93 automated checks passed, including the five factory regressions.

## Windows MultiBot startup and basic controls - 2026-09-30

The disposable Windows Cata client loaded the full patched donor addon. The
player confirmed no remaining Lua errors and working Stay, Follow and the
main Attack button. Initial attack requests reached the server but were
rejected outside the existing 25-yard owner/bot target gate; moving closer
confirmed engagement. That gate is temporary companion behavior, not the
intended final movement/range contract. This check does not establish managed
roster UI, lifecycle completion, ordinary-player permissions or other addon
feature families.

Two startup fixes were added to the reproducible compatibility patch:
LibDataBroker must load before LibDBIcon, and the spellbook uses Cata's
GetMacroIcons caller-owned table instead of removed GetNumMacroIcons and
GetMacroIconInfo globals. The latter follows the 4.3.4 Blizzard_MacroUI source
(RefreshPlayerSpellIconInfo). Candidate and installed modified Lua passed
Lua 5.1 syntax checks; patch application against the pinned donor passed.

## Managed capability advertisement and Windows client staging - 2026-09-30

HELLO now advertises ALT_ROSTER_V1 and BOT_LIFECYCLE_V1 only when the validated
player lifecycle service is enabled. Other capabilities remain absent. This
reports service availability, not requester permission: roster, start/stop and
poll authorization remain independently checked. Disabled/invalid module
settings still produce an empty capability set. The exact advertised list has
an automated regression check; combined worldserver build and all 88 checks
passed. Authserver was also built for the isolated runtime check.

The compatibility candidate was copied into a previously absent MultiBot
directory in the Windows Cata test client; no existing addon was overwritten.
Only addon source/assets/license were copied, not Git metadata or personal
saved variables. Actual client startup and integrated behavior remain pending.
Linux compatibility is required for the server/module, not the client addon.

## Portable handoff and donor response checks - 2026-09-30

The source review found no Windows API dependency in mod-playerbots/src or its
module CMake entry point. The core's modules/CMakeLists.txt already installs
module configs under CONF_DIR/modules on Unix. This is a source-level finding,
not a Linux build certification. The module README now distinguishes the
Windows-verified build from the pending Linux checkpoint and documents plain
CMake/CTest entry points. Addon preparation uses relative Git/Lua commands;
neither path depends on optional local PowerShell test helpers.

The Lua 5.1 mock now exercises the patched donor's actual response parser:
unexpected-sender rejection, encoded ALT_ROSTER entry decoding, batch-count
integrity, connect pending then ONLINE, and disconnect pending/STOPPING then
OFFLINE. Callbacks do not report completion for pending replies. Timers are
captured rather than run. Syntax and expanded mock checks passed. No C++ or
native runtime behavior changed in this documentation/test slice; the prior
combined build remains at 87 passing checks. Real client UI/native lifecycle
authorization and a clean Linux build remain separate validation work.

## Initial Cata client compatibility candidate - 2026-09-30

`addons/MultiBot/cata-compat.patch` applies to the refreshed upstream addon
`1eac0d9106b8cdf0a79da3974ee1f516f8ca3fbc`. It retains the complete donor
addon/UI/protocol implementation and upstream licensing, rather than replacing
it with a custom client. The patch changes Interface to 40300, keeps the older
party/raid roster events instead of GROUP_ROSTER_UPDATE, and registers MBOT
before sending. Missing/failed registration makes Comm.Send return false.
An isolated candidate checkout was prepared; installed client addons and saved
variables were not changed. The patch passes Git's application check against
the immutable donor revision.

The donor's main attack button in `UI/MultiBotAttackUI.lua` sends
`do attack my target`, not bare `attack`. That exact alias now routes to the
same existing attack request; role-filtered forms remain unsupported rather
than accidentally commanding every bot. Combined worldserver build and all
87 automated checks passed with the expanded command assertions.

Lua 5.1.5 syntax checks passed for all 127 candidate addon Lua files and the
new communication test. The test loads the actual patched donor Comm code
with mocked WoW globals and checks successful/failed/missing prefix registration,
self-whisper, party/raid selection, HELLO and PING. This does not prove real
frame-event compatibility, UI loading or client-server interaction. Prefix
registration and event choices still need confirmation in the Cata client.
Class/spec/talent data and unsupported feature UIs are not ported by this patch.
Keep optional capability advertisement off until that integrated pass.

## MultiBot basic group-control transport - 2026-09-30

Refreshed MultiBot-Chatless upstream default branch to
`1eac0d9106b8cdf0a79da3974ee1f516f8ca3fbc`. `Core/MultiBotEngine.lua`
ActionToTarget/ActionToGroup/ActionToTargetOrGroup route basic bot actions by
normal whisper/party/raid chat. `UI/MultiBotLeftCoreUI.lua` emits stay/follow
through this path. The bridge's COMBAT endpoint instead handles combat
strategy toggles and wait-for-attack settings; POSITION handles disperse.
Neither is repurposed as a generic attack/follow endpoint in this port.

The existing bot-whisper parser is now shared with a group-chat hook for
follow, stay/hold, attack and stop/cease. Only native party/raid channels,
in-world human senders and current group membership are eligible. /party in
a raid reaches only the sender's subgroup; /raid reaches the raid. BG and
addon-language messages are excluded. Each selected bot must be in the routed
group and pass existing full-control checks; PlayerbotControl::Dispatch checks
again before posting a request. Managed account links are not a substitute
for temporary party control. Normal human chat delivery is preserved.

The group reply summarizes queued requests and rejections, not completed
movement or combat. Strategy mutation, flee, disperse, loot and other donor
commands remain unsupported; unrecognized text does not dispatch anything.
No invented addon opcode, console invocation or new lifecycle path was added.
The combined worldserver build and all 87 automated checks passed, including
shared command-vocabulary and party-versus-raid routing checks. Integrated
runtime authorization and MultiBot client behavior remain unverified.

## MultiBot offline managed roster - 2026-09-30

Upstream bridge HEAD was rechecked at
`1da05982e478cb00e0b6c87314afe7e0e9653ffb`. The protocol basis is
SendAltRosterPackets and ConsumeAltRosterRequestRateLimit in
`src/MultiBotBridge.cpp`, checked against the addon's strict batch reader.
`GET~ALT_ROSTER` now returns donor-compatible BEGIN/ENTRY/END frames:
count/truncated boundaries and guid/encoded-name/class/level/ONLINE-or-OFFLINE
entries. The batch is bounded at 128 entries and each native message at 250
bytes; omissions and overflow set the truncation flag rather than pretending
the batch is complete. Entries are ordered by native character GUID.

Cata sources this list from PlayerbotManagedControl::ListFor, not the donor's
same-account SQL query. Only explicitly configured, account-linked characters
with current authorization appear, including offline identities; account IDs
are not transmitted. An empty authorized list produces a valid empty batch.
Loaded native receipts remain ONLINE while stop is pending and become OFFLINE
after closure. Loading is OFFLINE for this two-state discovery schema; lifecycle
polling supplies the more precise CONNECTING state.

The donor's four roster requests per two seconds is enforced using the bounded
account-keyed requester storage already used by mutation protection. Rejected
queries return ERR without starting a batch. Client adaptation, gameplay
endpoints and integrated live validation remain pending; optional capabilities
are still not advertised. No lifecycle scheduler, SQL or account provisioning
was added.
The combined worldserver build and all 85 automated checks passed. The new
checks exercise roster framing, truncation, presence and query-rate boundaries;
live authorization and client behavior still need the integrated test.

## MultiBot managed lifecycle endpoints - 2026-09-30

The same bridge upstream HEAD was rechecked and remains
`1da05982e478cb00e0b6c87314afe7e0e9653ffb`. Protocol basis:
HandleBridgeOpcode, SendBotLifecycleResultPacket, SendBotLifecycleStatePacket
and the donor mutation-token/rate contract in `src/MultiBotBridge.cpp`.
Requests now include `RUN~BOT_CONNECT~guid~token`,
`RUN~BOT_DISCONNECT~guid~token` and `GET~BOT_LIFECYCLE_STATE~guid~token`.
Replies retain donor BOT_LIFECYCLE/BOT_LIFECYCLE_STATE field order and encoded
names. The adapter calls PlayerbotManagedControl, never console commands.

Every mutation rechecks existing managed admission policy; every poll rebuilds
the authorized list. Revoked links and other-party control expose neither names
nor receipts. FORBIDDEN replies are access denials, not proof a bot logged out.
Accepted starts/stops report PENDING. Only native receipt transitions establish
ONLINE/OFFLINE. Pending teardown uses donor-compatible CONNECTING with STOPPING
reason because the donor poll parser has no DISCONNECTING state. Pre-teardown
login failure also remains pending until the session is closed.

Replay/rate storage is world-thread-only and account-keyed, without retained
Player/session pointers: 64 mutation attempts per two seconds, 320 retained
tokens per account for two minutes, at most 256 requester accounts. Inactive
accounts expire after ten minutes; full storage rejects rather than evicting
live replay protection. Native addon throttling still applies to all requests.
Unlike donor manager state, Cata receipts supply completion and reserve native
session capacity; no parallel pending-login scheduler is introduced. Repeat
connects with a different token currently use native admission rejection rather
than the donor's ALREADY_ONLINE/ALREADY_CONNECTING success shortcuts.

BOT_LIFECYCLE_V1 is deliberately not advertised yet: offline ALT_ROSTER,
client adaptation and integrated validation remain pending. Gameplay endpoints
are also pending. The automated checks cover parsing, receipt-state mapping and
bounded replay/rate behavior, not live lifecycle authorization.
The combined worldserver build and all 81 automated checks passed for this slice.

## Initial MultiBot transport - 2026-09-29

Refreshed `Wishmaster117/mod-multibot-bridge` upstream default branch at
`1da05982e478cb00e0b6c87314afe7e0e9653ffb`. The protocol basis is
`MultiBotBridge.cpp` HandleBridgeOpcode, SendAddonPacket, BuildRosterPayload
and the 255-byte envelope budget. This is a small Cata adapter of that contract,
not a wholesale copy of the WotLK bridge implementation.

Cata decodes the MBOT prefix separately; requests are `HELLO~1`, `PING~token`
and `GET~ROSTER`. Replies use native Cata addon packets sent privately to the
requester, including for party/raid requests. The core seam runs on the world
thread after native channel, throttle and routing checks. Self-whispers and
valid non-BG party/raid channels are supported; other recipients/channels
keep native routing. Module-off builds have a no-op seam.

`Playerbots.MultiBot.Enabled` is default-off and fails closed with invalid
module configuration. Handshake advertises an empty capability set. Roster
uses existing full-control authorization and the donor's name/class/level/
map/alive/health/mana columns; it does not disclose accounts or offline
identities. Oversized rosters fail explicitly rather than silently truncating.
No database or lifecycle mutation is reachable here. Donor command endpoints,
managed lifecycle polling, larger roster framing and the Cata client addon
adaptation remain pending. Automated parser checks do not prove client behavior.
The combined worldserver build and all 77 automated checks passed for this slice.

## Managed player lifecycle authority - 2026-09-29

`Bot/Cmd/PlayerbotManagedControl` supplies world-thread ListFor/Start/Stop for
later player transports. It re-resolves the configured identity, native
character cache, requester session and current party relationship on each call.
Accepted native requests return an immutable view of their lifecycle receipt.
No Player/session pointers survive a request. The list omits managed account IDs.

The source basis is `PlayerbotMgr.cpp` account-link admission and
`Mgr/Security/PlayerbotSecurity.cpp` master/GM relationships at upstream master
`7bae1b5c58c76a0aa20381155edc08096d1485b2`, refreshed 2026-09-29. The Cata
adaptation uses a bounded directional config list of humanAccount:botAccount
pairs instead of importing the donor's linked-account database table. Same-account
multicharacter admission is excluded because native Cata owns one session per
account. Guild/random-account, gearscore and population eligibility are deferred.

Player lifecycle access is separately default-off. Ordinary access requires a
valid managed identity, an in-world human, trusted link and same faction; listing
or stopping a grouped bot additionally requires existing full party control.
GM overrides do not bypass the feature gate, native identity or human checks.
Managed.Enabled gates new starts, while authorized list/stop remains possible
with admission disabled. Existing console operations retain their separate
authority. An account link never grants gameplay control or adopts a master.

Four new regression cases cover policy prerequisites, link/party separation,
direction and revocation, and malformed/excessive links. The combined worldserver
build and all 75 tests passed. Client transport and native runtime authorization
checks remain pending; this slice introduces no new player command or endpoint.

## Managed lifecycle receipts - 2026-09-29

The latest managed admission now retains a session-owned receipt containing
atomic flags and no Player/session pointers. Core publishes native login
completion, known login failures, explicit stop requests and final session
closure after logout/save returns. The configured roster retains the receipt
after session deletion and across reloads of the same account/character binding.
A new admission gets a new receipt, isolating late observations of old attempts.
`managed list` uses this state instead of interpreting any non-null Player as
a completed login. Stopping an existing development session also captures its
receipt; stop remains available with managed admission disabled.

The source basis is `PlayerbotMgr.cpp` and
`Script/WorldThr/{PlayerbotOperations.h,PlayerbotWorldThreadProcessor.cpp}` at
upstream `7bae1b5c58c76a0aa20381155edc08096d1485b2`. The audit found the donor's
load, queued registration and native logout to be separate boundaries; an
immediate command `ok` is not an admission result. This receipt is a Cata-native
adaptation of those boundaries, not a copied donor operation queue or a durable
database ledger. It does not add account creation or ordinary-player connect
permission. Native asynchronous database commit success is outside its contract.

Four regression cases cover successful login/exit, failure/cancellation during
loading, unexpected closure/shutdown and receipt identity across reload/retry.
Build and automated validation are recorded in the core's current roadmap.
The configured lifecycle path still needs an integrated runtime check.

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

The matching core's `doc/local/playerbots/PLAYERBOTS_REFERENCE_GUIDE.md`
records source authority and secondary Cata comparisons. Modern Playerbots
remains the architecture donor; ArkCORE NPC bots are a behavior reference only.
That reference review imported no code and does not establish feature parity.

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

## Managed existing-character roster, first slice - 2026-09-29

The source basis is upstream `src/Bot/PlayerbotMgr.{h,cpp}` at
`7bae1b5c58c76a0aa20381155edc08096d1485b2`: separate bot identity from
its temporary master, reject duplicates, and enter through native session
loading. This is an adaptation of that ownership contract, not a copy of its
query holder, guild/account-link permissions, command parser or random manager.
Those donor services depend on WotLK-specific systems not yet present here.

`PlayerbotManagedRoster` parses a bounded, unique set of configured
account ID/character GUID pairs. The settings are default-off. Core
`World::TryStartServerOriginPlayerbot` checks the existing character cache,
account ownership, security, current online state, session and player limits,
then uses the already established asynchronous Cata login path. It refuses to
replace a human session. The numbered development slots now call this same
entry point. A matching world-thread stop request uses the normal save/logout
path.

The combined worldserver build and 67 automated checks passed for this slice.
The configured admission path itself still needs a disposable runtime check
before describing its start/stop behavior as client-confirmed.

The first management transport is console-only
`server playerbotdev managed <list|start GUID|stop GUID>`. Listing reads the
native character cache and active session; it never treats a party controller
as an account owner. No ordinary player or addon connect permission is exposed
by this command. Accepted start and stop requests report admission or queued
exit, not confirmed completion. The enable flag gates new admissions; listing
and stopping configured bots remain available when it is off. A later
lifecycle manager needs durable
pending/completed/failed outcomes, human-facing authorization, account
provisioning and MultiBot mapping. This first slice has not had a live
managed-roster session test.
