# Cataclysm Playerbots

An experimental port of [AzerothCore mod-playerbots](https://github.com/mod-playerbots/mod-playerbots)
to TrinityCore Cataclysm 4.3.4. The goal is the familiar Playerbots feature set
on Cata, using upstream behavior wherever it fits the native server.

Use the matching [iotadev TrinityCore fork](https://github.com/iotadev/TrinityCore).
The module depends on that fork's session and module hooks; stock TrinityCore
does not provide them. The core README records its corresponding module revision.
Building this repository alone does not produce a server.

## Status

The immediate target is a useful human-led dungeon party. The longer-term goal
is upstream Playerbots functionality on Cata, including autonomous bots. This
alpha is not yet a complete dungeon or autonomous-population implementation.

| Area | Implemented scope |
| --- | --- |
| Lifecycle | Native sessions, managed rosters, authorized login/logout and bounded character creation |
| Shared AI | Donor-shaped combat/noncombat/dead engines, queued-action cleanup, follow/stay/attack, roles and recovery |
| Warrior, Mage, Priest | Starter Warrior/Mage spec routes; Priest healing, buffs, cures, resurrection and conservative damage |
| Rogue, Hunter | Default-off starter combat routes; native owned Hunter pet control; no live combat validation yet |
| Inventory | Gear inspection, explicit one-change equip/bag upgrades, guarded native loot votes and item retention |
| Human-led quests | Native shares, selected-giver acceptance, log inspection and explicit rewards |
| Maintenance | Selected-NPC repair, recovery-supply purchases, limited gray-junk sales and class training |
| MultiBot | Cata loading patch, managed roster/lifecycle and a subset of party/strategy controls |

Class rotations and high-level coverage remain incomplete. Rogue poisons/stealth,
Hunter taming, Rogue/Hunter gear scoring, autonomous quest travel, population
management and automatic group formation are not implemented. MultiBot's full
upstream UI includes buttons the server does not yet support.

See the [command guide](docs/USAGE.md), [MultiBot preparation](addons/MultiBot/README.md)
and the core's [roadmap](https://github.com/iotadev/TrinityCore/blob/master/doc/local/playerbots/PLAYERBOTS_PORT_ROADMAP.md).
Optional passive engine history supports a separate read-only context project;
that application is not included or required to run bots.

## What has been verified

Outdoor and Ragefire checks observed a four-bot Warrior/Mage/Priest party
engaging, healing, recovering, looting and entering the same dungeon instance.
Separate checks observed MultiBot login/logout/reconnect, one saved equipment
change, a controlled need/greed/pass award, four bots completing quest objectives
and one explicit saved quest reward. A full dungeon clear remains unverified.

| Dated source validation | Result |
| --- | --- |
| October 13, Windows and Linux, all three installed optional modules enabled | worldserver/tests-common built; 476/476 tests on each platform |
| Windows, all three installed optional modules disabled | worldserver/tests-common built; 20/20 tests |

The third installed module was the separate context exporter. Linux builds used
Ubuntu 22.04/GCC 11.4 with normal PCH; Linux realm runtime remains untested.
Later documentation/helper cleanup and the include-only no-PCH correction have
not received a new full native build. These are dated source results, not CI
badges for every subsequent commit.

Rogue/Hunter combat and the prepared six-bot outdoor raid remain untested.
Maintenance purchases/sales/training, bot-initiated bag replacement and broader
inventory persistence also need live qualification. Detailed build scope,
runtime evidence and remaining limits are retained in [PORTING.md](PORTING.md).

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

Admission is still a manual development setup. Read the template's Dev/Managed
identity and account-link settings before enabling it; configured bot accounts
are reserved from human-client login while development bots are enabled.
The optional factory has separate [schema and enrollment requirements](sql/README.md).
There is no automatic account population or one-click playable setup.

## Contributing and provenance

Cata API comparisons, donor feature ports and repeatable bug reports are useful
contributions. Include the core/module revisions, bot class and level, and the
behavior observed. Keep credentials, client files and database dumps out of
reports. Follow [PORTING.md](PORTING.md) when documenting donor source and
adaptations so future upstream changes remain traceable.

Retained source headers specify GPL version 2 or later. This port preserves the
license and author credits; see [LICENSE](LICENSE) and [AUTHORS.md](AUTHORS.md).
