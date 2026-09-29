/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_PLAYERBOT_ROSTER_H
#define CATA_PLAYERBOTS_PLAYERBOT_ROSTER_H

#include "ObjectGuid.h"
#include <string>
#include <vector>

class Player;

struct PlayerbotRosterEntry
{
    ObjectGuid Guid;
    std::string Name;
    uint8 Class = 0;
    uint8 Level = 0;
};

// Active, controllable bots only. Offline managed characters need a factory
// and persistent ownership data before they can appear in this roster.
class PlayerbotRoster
{
public:
    static std::vector<PlayerbotRosterEntry> ListActiveFor(Player& requester);
};

#endif
