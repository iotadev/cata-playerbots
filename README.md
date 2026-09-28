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

The most recent mixed-party playtest reached Ragefire Chasm and observed Mage
damage and sustained Priest healing. It did **not** establish a complete dungeon
clear, dependable death recovery, tank threat, or full class/spec rotations.
Bot accounts and characters are still supplied manually; autonomous population,
questing, and world progression have not been ported.

The port is moving from the temporary companion logic to upstream Playerbots'
engine, contexts, actions, triggers, values, and strategies. Imported components
and Cata-specific changes are tracked in [PORTING.md](PORTING.md). An imported
component is not necessarily wired into live bot decisions.

## Building a matching development checkout

Place this repository at `modules/mod-playerbots` inside a **matching** core
checkout. The core build discovers it as an optional static module. From the
core root, configure with `-DMODULE_MOD_PLAYERBOTS=ON` (or `OFF` to exclude it).
Runtime bot admission is disabled by default.

The module provides `conf/playerbots.conf.dist`. In the current development
layout, an active `modules/playerbots.conf` belongs beside `worldserver.conf`;
`Modules.ConfigDirectory` can select a different directory. Copy and edit the
template only in a disposable test realm. Building the module does not create
accounts or characters.

These are development notes, **not** complete installation instructions. This
repository alone cannot reproduce the tested server; use the matching core and
the separately versioned AHBot module if you want its economy behavior.

## Contributing and provenance

If you're working on Cata bots, compare notes with us. The most useful help
right now is reviewing Cata API differences or porting a class/spec from the
upstream module. Bug reports should name the core and module revisions, the
bot's class and level, and the behavior you saw. Keep credentials, client
files, database dumps, and private-server packages out of issues.

The upstream Playerbots code remains GPLv2; this port keeps its license and
author credits. [PORTING.md](PORTING.md) records the donor revision and the
changes needed for Cata. See also [LICENSE](LICENSE) and
[AUTHORS.md](AUTHORS.md).
