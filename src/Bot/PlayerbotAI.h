/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_PLAYERBOTAI_H
#define PLAYERBOTS_PLAYERBOTAI_H

#include "ObjectGuid.h"

class Creature;
class Player;
class WorldSession;

// Session-bound Cata adapter for the donor engine's PlayerbotAI* interfaces.
// It never owns or retains a Player pointer. Accessors are map-thread only.
class PlayerbotAI
{
public:
    explicit PlayerbotAI(WorldSession& session) : session(session) { }
    Player* GetBot() const;
    Creature* GetCurrentTarget() const;
    void SetCurrentTarget(ObjectGuid guid) { targetGuid = guid; }
    void ClearCurrentTarget() { targetGuid.Clear(); }

private:
    WorldSession& session;
    ObjectGuid targetGuid;
};

#endif
