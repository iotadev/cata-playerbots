/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#ifndef PLAYERBOT_PARTY_SUPPORT_H
#define PLAYERBOT_PARTY_SUPPORT_H
#include <algorithm>
#include <cstdint>
#include <vector>

class Player;
namespace PlayerbotPartySupport
{
inline bool CanCure(bool enabled, bool alive, bool learned, bool nativeEligible)
{
    return enabled && alive && learned && nativeEligible;
}
// Eligibility is supplied by the caller. Unlike healing, cures must not drop
// full-health members. Stable ties and cast rejection fallback are preserved.
template <typename Candidate, typename Health, typename Attempt>
bool TryInPriorityOrder(std::vector<Candidate>& candidates, Health&& health, Attempt&& attempt)
{
    std::stable_sort(candidates.begin(), candidates.end(), [&](Candidate const& left, Candidate const& right)
    {
        return health(left) < health(right);
    });
    for (Candidate const& candidate : candidates)
        if (attempt(candidate))
            return true;
    return false;
}
std::vector<Player*> Candidates(Player& bot, Player* owner);
bool HasDispellableAura(Player& bot, Player& target, std::uint32_t spellId, std::uint32_t dispelType);
}
#endif
