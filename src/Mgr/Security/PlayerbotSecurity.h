/*
 * Adapted from AzerothCore mod-playerbots PlayerbotSecurity.h at
 * 8827dd6fcbb2bb25988787a40f06fc93daf8e02d. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_PLAYERBOT_SECURITY_H
#define CATA_PLAYERBOTS_PLAYERBOT_SECURITY_H

#include "Define.h"

class Player;

enum PlayerbotSecurityLevel : uint32
{
    PLAYERBOT_SECURITY_DENY_ALL = 0,
    PLAYERBOT_SECURITY_TALK = 1,
    PLAYERBOT_SECURITY_INVITE = 2,
    PLAYERBOT_SECURITY_ALLOW_ALL = 3
};

// A server-side policy for bot relationships. It does not retain Player
// pointers; evaluate it in the caller's current player/map context.
class PlayerbotSecurity
{
public:
    explicit PlayerbotSecurity(Player& bot) : bot(bot) { }

    PlayerbotSecurityLevel LevelFor(Player const& from) const;
    bool CheckLevelFor(PlayerbotSecurityLevel level, Player const& from) const
    {
        return LevelFor(from) >= level;
    }

private:
    Player& bot;
};

#endif
