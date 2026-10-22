# Playerbots commands and optional features

Use this with the [overview and setup](../README.md) and
[configuration template](../conf/playerbots.conf.dist). Features below require
an admitted bot, an authorized controller and their individual configuration
gates. A queued request is not confirmation that a native operation completed.

## Equipment and item retention

`gear?` inspects a bounded recent survey with
`Playerbots.Equipment.StarterScore.Enabled`. `gear apply` additionally requires
`Playerbots.Equipment.StarterEquip.Enabled` and attempts one native inventory
change while idle. The donor-derived scoring model covers supported
Warrior/Mage/Priest specs at levels 10–39; Rogue/Hunter scoring is not implemented.
Unknown item facts remain unresolved.

Optional `Playerbots.Loot.Rolls.Enabled` uses native roll eligibility and
qualified item facts for need/greed/pass. It requires StarterScore and
`Playerbots.Loot.PassOnGroupLoot = 0`. Voting does not equip the award.
Profession tools, relevant materials, unread recipes and qualified disenchant
candidates have positive retention rules; these do not enable crafting or
disenchanting and do not authorize general vendor sales.

## Maintenance

`sell vendor` is available under the separate default-off
`Playerbots.Maintenance.Sell.Enabled` gate. At the selected nearby vendor it
sells at most eight owned backpack/bag stacks of plain gray misc/junk. This
limited subset requires neither player to have relevant professions or a guild,
complete quest metadata with no active item reference, and no bindings, effects,
affixes, scripts or special flags. Completed-quest turn-in stacks stay protected.
Equipment, materials and unresolved items are not eligible. The native handler
owns inventory/money/buyback; finite buyback is not rollback. Replies report
observed removal and money gains, not saved inventory. Client sale qualification
is pending; [porting record](../PORTING.md) records current build evidence.

With `Playerbots.Maintenance.Repair.Enabled = 1`, select a nearby repair NPC
and whisper a controlled bot `repair`, or use party/raid chat with the usual
subgroup scope. Native repair-all uses bot funds and reputation discount;
insufficient money can leave partial repairs. Replies report observed equipped
durability and spending.

Purchases use the independent default-off
`Playerbots.Maintenance.Buy.Enabled` gate. At a selected nearby vendor,
`buy <numeric item ID>` requests one native food/drink/recovery-supply bundle,
capped at two carried stacks of that ID. `buy vendor` chooses needed food,
drink, healing potions, mana potions and bandages independently using
donor current/better-stock policy, higher-level usable items first. It scans
at most 128 rows and attempts at most ten bundles per type, rechecking need
after each. Native blocking stops that type without lower-rank fallback.
Potions must pass the existing combat-potion classifier; flasks and channeled
potions are excluded. Native item prerequisites still apply. Stocking bandages
does not add automated bandage use; potion use keeps its separate enable gate.

Both players must be eligible to interact with the captured NPC. Requests expire
after five seconds and recheck control, location and idle state on the world
owner. Cata owns vendor conditions, stock, discounts, payment and storage.
Item/currency costs are excluded. Summaries report observed inventory increases
and spending, not saved inventory or a fully stocked guarantee. The donor lets
bots with a real human master use current money. Autonomous spending reserves,
NPC discovery, scheduling and general shopping remain separate.


## Human-led quests

Quest settings under `Playerbots.Quest` default off. The current controller can
use these commands by whisper, or authorized party chat where noted:

| Command | Gate | Behavior |
| --- | --- | --- |
| Native client quest share | AcceptShared.Enabled | Accept eligible incoming human shares through native handlers |
| `accept <quest>` / `accept *` | AcceptNpc.Enabled | Selected nearby giver; batch copies at most 25 offers and rechecks each |
| `quests [all\|completed\|incompleted\|summary]` | Inspection.Enabled | Read native active-log facts; `co`/`in` are filter aliases |
| `reward <quest> <item>` / `reward *` | Reward.Enabled | Explicit native reward; batch skips quests with several choice items |
| `share <quest>` | Share.Enabled | Whisper one bot to submit a native party offer; not proof of recipient acceptance |
| `drop <quest>` | Abandon.Enabled | Separate destructive opt-in; whisper one active quest only |

Quest/item operands accept numeric IDs or native links. Reward uses an item
entry, not a UI slot; item `0` is valid only for no-choice rewards. `reward *`
attempts at most 25 active IDs at the selected giver, with zero or one choice.
Mutation requests require fresh control, state and location checks. Partial
batches are not rolled back or automatically retried. No forced completion,
automatic travel, reward-choice guessing or rewarded-history reset is included.

`Playerbots.Quest.SyncLootWithPlayer.Enabled` optionally defers competitive
quest-class corpse items the human still needs. The native item's per-player
flag exempts shared drops; the group's loot method is not that flag. Deferral
does not reserve an item or change native recipient eligibility.

## Party and addon controls

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

For the [Cata MultiBot addon candidate](../addons/MultiBot/README.md), enable the
server bridge with `Playerbots.MultiBot.Enabled`. Structured strategy mutations
also require `Playerbots.StrategyControl.AddonMutations`; group requests require
the additional `Playerbots.StrategyControl.GroupMutations` gate. ALL selects
authorized controlled bots, GROUP/PARTY the current group including raids, and
RAID requires a raid. Execution rechecks control, login, group, phase and idle
conditions. Unresolved timeouts are unknown outcomes; refresh STATE before
retrying a toggle. Completed changes are not rolled back.

The full donor addon contains features the server has not ported. Its presence
does not imply class/talent data parity or support for every button.

## Bag-capacity inspection

Ordinary bag-capacity improvements are recognized by shared item-usage and
quest-reward inspection. This is positive retention only: specialized bags and
unknown layouts are not replacements. Explicit `gear apply` can now fill one
empty carried bag slot or upgrade the smallest qualified ordinary carried bag
as a fallback after gear, using an owned empty ordinary backpack candidate.
Cata owns equip/storage checks; the displaced empty bag
stays in the backpack. Confirmation checks bag/content GUIDs, counts and affix
facts. Existing equip gates apply. Populated replacement requires the matching
local core's swap-scoped storage adaptation; generic storage remains guarded.
A human-reported manual populated-bag swap worked; bot-initiated replacement
is not yet client-qualified. Specialized/affixed bags, bank/nested/busy
layouts and bag loot-vote/purchase support remain excluded. Native replacement
and persistence still need client qualification.

## Class training

Local class training supports `trainer` to list learnable offers,
`trainer learn` for a bounded purchase batch, or `train <numeric trainer spell ID>` at
the human's selected nearby default class trainer, behind the independent
default-off `Playerbots.Maintenance.Train.Enabled` gate. Native Cata handlers
own eligibility, payment and learning. Inspection scans 128 rows and shows up to
12 offers. Batches attempt at most 16 initially available offers, cheapest first.
Funds/state/caps or unconfirmed learning stop the batch without retry/rollback.
Trainer travel and profession setup are not implemented; addon trainer UI is not connected.
Replies distinguish newly observed learned spells from submission. Client
training and persistence still need qualification; see [porting record](../PORTING.md).
