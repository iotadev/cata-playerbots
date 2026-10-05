/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef CATA_PLAYERBOTS_PLAYERBOT_CONTROL_H
#define CATA_PLAYERBOTS_PLAYERBOT_CONTROL_H

#include "ObjectGuid.h"
#include "PlayerbotStrategyBinding.h"
#include <string>

class Player;

enum class PlayerbotControlCommand
{
    Follow,
    Hold,
    Stay,
    Attack,
    Cease,
    Rebuff
};

enum class PlayerbotControlResult
{
    Queued,
    BotUnavailable,
    Unauthorized,
    NotFollowing,
    Busy,
    InvalidCommand
};

// World-thread entry point shared by future chat and addon transports. The
// session mailbox hands accepted commands to the bot's map update.
class PlayerbotControl
{
public:
    static PlayerbotControlResult Dispatch(Player& requester, ObjectGuid botGuid, PlayerbotControlCommand command);
    static PlayerbotControlResult DispatchRange(Player& requester, ObjectGuid botGuid, std::string const& param);
    static PlayerbotControlResult DispatchStrategy(Player& requester, ObjectGuid botGuid, std::string const& command,
        std::string const& token = {}, std::string const& target = {}, uint64 batch = 0,
        PlayerbotStrategyBinding const& binding = {});
};

#endif
