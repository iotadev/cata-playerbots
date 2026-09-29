/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotControl.h"
#include "PlayerbotSecurity.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"

PlayerbotControlResult PlayerbotControl::Dispatch(Player& requester, ObjectGuid botGuid, PlayerbotControlCommand command)
{
    WorldSession* botSession = sWorld->FindServerOriginPlayerbot(botGuid);
    Player* bot = botSession ? botSession->GetPlayer() : nullptr;
    if (!bot || !bot->IsInWorld() || bot->GetGUID() != botGuid)
        return PlayerbotControlResult::BotUnavailable;

    if (!PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, requester))
        return PlayerbotControlResult::Unauthorized;

    uint32 requesterGuidLow = requester.GetGUID().GetCounter();
    bool gm = requester.CanBeGameMaster();
    switch (command)
    {
        case PlayerbotControlCommand::Follow:
            if (gm)
                botSession->RequestServerOriginFollow(requesterGuidLow);
            else
                botSession->RequestPartyControllerFollow(requesterGuidLow);
            break;
        case PlayerbotControlCommand::Hold:
            if (gm)
                botSession->RequestServerOriginHold();
            else
                botSession->RequestPartyControllerHold();
            break;
        case PlayerbotControlCommand::Attack:
            if (botSession->GetServerOriginFollowTargetGuidLow() != requesterGuidLow)
                return PlayerbotControlResult::NotFollowing;
            botSession->RequestServerOriginAttack();
            break;
        case PlayerbotControlCommand::Cease:
            botSession->RequestServerOriginCease();
            break;
    }
    return PlayerbotControlResult::Queued;
}
