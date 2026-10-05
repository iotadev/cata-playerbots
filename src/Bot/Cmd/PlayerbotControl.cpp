/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotControl.h"
#include "PlayerbotStrategyControl.h"
#include "PlayerbotSecurity.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include "../../Ai/Base/PlayerbotCombatMovement.h"
#include "../../Script/PlayerbotConfig.h"

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
        case PlayerbotControlCommand::Stay:
            if (command == PlayerbotControlCommand::Stay && PlayerbotModuleStayEnabled())
            {
                if (botSession->GetServerOriginFollowTargetGuidLow() != requesterGuidLow &&
                    botSession->GetServerOriginPartyControllerGuidLow() != requesterGuidLow)
                    return PlayerbotControlResult::NotFollowing;
                return botSession->RequestPlayerbotStay(requesterGuidLow) ? PlayerbotControlResult::Queued : PlayerbotControlResult::Busy;
            }
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
        case PlayerbotControlCommand::Rebuff:
            return botSession->RequestPlayerbotRebuff(requesterGuidLow) ? PlayerbotControlResult::Queued : PlayerbotControlResult::Busy;
    }
    return PlayerbotControlResult::Queued;
}
PlayerbotControlResult PlayerbotControl::DispatchRange(Player& requester, ObjectGuid botGuid, std::string const& param)
{
    if (PlayerbotCombatMovement::ParseRangeCommand(param).Operation == PlayerbotCombatMovement::RangeOperation::Invalid)
        return PlayerbotControlResult::InvalidCommand;
    WorldSession* session = sWorld->FindServerOriginPlayerbot(botGuid);
    Player* bot = session ? session->GetPlayer() : nullptr;
    if (!bot || !bot->IsInWorld() || bot->GetGUID() != botGuid) return PlayerbotControlResult::BotUnavailable;
    if (!PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, requester))
        return PlayerbotControlResult::Unauthorized;
    return session->RequestPlayerbotRange(requester.GetGUID().GetCounter(), param) ?
        PlayerbotControlResult::Queued : PlayerbotControlResult::Busy;
}
PlayerbotControlResult PlayerbotControl::DispatchStrategy(Player& requester, ObjectGuid botGuid, std::string const& command,
    std::string const& token, std::string const& target, uint64 batch, PlayerbotStrategyBinding const& binding)
{
    if (!PlayerbotStrategyControl::Parse(command)) return PlayerbotControlResult::InvalidCommand;
    if (!PlayerbotModuleStrategyControlEnabled()) return PlayerbotControlResult::BotUnavailable;
    WorldSession* session = sWorld->FindServerOriginPlayerbot(botGuid);
    Player* bot = session ? session->GetPlayer() : nullptr;
    if (!bot || !bot->IsInWorld() || bot->GetGUID() != botGuid) return PlayerbotControlResult::BotUnavailable;
    if (!PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, requester))
        return PlayerbotControlResult::Unauthorized;
    if (session->GetServerOriginFollowTargetGuidLow() != requester.GetGUID().GetCounter() &&
        session->GetServerOriginPartyControllerGuidLow() != requester.GetGUID().GetCounter())
        return PlayerbotControlResult::NotFollowing;
    return session->RequestPlayerbotStrategy(requester.GetGUID().GetCounter(), command, token, target, batch, binding) ?
        PlayerbotControlResult::Queued : PlayerbotControlResult::Busy;
}
