/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_PLAYERBOT_CONTROL_H
#define CATA_PLAYERBOTS_PLAYERBOT_CONTROL_H

#include "ObjectGuid.h"

class Player;

enum class PlayerbotControlCommand
{
    Follow,
    Hold,
    Attack,
    Cease
};

enum class PlayerbotControlResult
{
    Queued,
    BotUnavailable,
    Unauthorized,
    NotFollowing
};

// World-thread entry point shared by future chat and addon transports. The
// session mailbox hands accepted commands to the bot's map update.
class PlayerbotControl
{
public:
    static PlayerbotControlResult Dispatch(Player& requester, ObjectGuid botGuid, PlayerbotControlCommand command);
};

#endif
