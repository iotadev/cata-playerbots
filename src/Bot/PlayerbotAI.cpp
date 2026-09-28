/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotAI.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "WorldSession.h"

Player* PlayerbotAI::GetBot() const
{
    if (!session.IsServerOrigin())
        return nullptr;
    Player* bot = session.GetPlayer();
    if (!bot || !bot->IsInWorld() || bot->GetGUID() != session.GetServerOriginCharacterGuid())
        return nullptr;
    return bot;
}

Creature* PlayerbotAI::GetCurrentTarget() const
{
    Player* bot = GetBot();
    return bot && !targetGuid.IsEmpty() ? ObjectAccessor::GetCreature(*bot, targetGuid) : nullptr;
}
