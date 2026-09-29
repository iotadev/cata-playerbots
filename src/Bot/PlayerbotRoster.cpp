/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotRoster.h"
#include "PlayerbotSecurity.h"
#include "Player.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>

std::vector<PlayerbotRosterEntry> PlayerbotRoster::ListActiveFor(Player& requester)
{
    std::vector<PlayerbotRosterEntry> result;
    for (WorldSession* session : sWorld->GetServerOriginPlayerbotSessions())
    {
        Player* bot = session->GetPlayer();
        if (!bot || !bot->IsInWorld() || session->GetServerOriginCharacterGuid() != bot->GetGUID() ||
            !PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, requester))
            continue;

        result.push_back({ bot->GetGUID(), bot->GetName(), bot->getClass(), bot->getLevel() });
    }
    std::sort(result.begin(), result.end(), [](PlayerbotRosterEntry const& left, PlayerbotRosterEntry const& right)
    {
        if (left.Name != right.Name)
            return left.Name < right.Name;
        return left.Guid.GetCounter() < right.Guid.GetCounter();
    });
    return result;
}
