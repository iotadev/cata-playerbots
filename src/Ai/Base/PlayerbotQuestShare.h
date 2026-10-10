/* Adapted from donor AcceptQuestShareAction at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_QUEST_SHARE_H
#define PLAYERBOT_QUEST_SHARE_H
#include "Define.h"
#include <limits>
class WorldSession;
namespace PlayerbotQuestShare
{
enum class Route { Unavailable, Ordinary, PartyConfirmation };
inline Route ChooseRoute(uint32 quest, bool turnIn, bool pushedToParty, bool activeForSharer, bool canShare)
{
    if (!quest || turnIn) return Route::Unavailable;
    if (pushedToParty)
        return activeForSharer && quest <= uint32(std::numeric_limits<int32>::max()) ? Route::PartyConfirmation : Route::Unavailable;
    return canShare ? Route::Ordinary : Route::Unavailable;
}
// World-owner only; the native session owns the actual pending share. Never
// retain a Player/Quest pointer or synthesize a pending invitation.
class Attempt
{
public:
    bool Take(uint64 sharer, uint32 quest)
    {
        if (!sharer || !quest) { Reset(); return false; }
        if (_sharer == sharer && _quest == quest) return false;
        _sharer = sharer; _quest = quest;
        return true;
    }
    void Reset() { _sharer = 0; _quest = 0; }
private:
    uint64 _sharer = 0;
    uint32 _quest = 0;
};
void Update(WorldSession& session, Attempt& attempt);
}
#endif
