/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_PRIEST_STRATEGY_H
#define PLAYERBOT_PRIEST_STRATEGY_H

#include <algorithm>
#include <vector>

class Player;

namespace PlayerbotPriest
{
enum class HealTier { None, Renew, Heal, Emergency };
inline HealTier TierForHealth(float healthPct)
{
    if (healthPct < 55.0f)
        return HealTier::Emergency;
    if (healthPct < 80.0f)
        return HealTier::Heal;
    if (healthPct < 90.0f)
        return HealTier::Renew;
    return HealTier::None;
}

// Candidates are already checked for life, map, range and line of sight.
// Failed casts must not starve the rest of the injured party. Stable ordering
// preserves the caller's tie preference, and one successful cast ends the pass.
template <typename Candidate, typename Health, typename Attempt>
bool TryInHealthOrder(std::vector<Candidate>& candidates, Health&& health, Attempt&& attempt)
{
    std::erase_if(candidates, [&](Candidate const& candidate)
    {
        float pct = health(candidate);
        return !(pct > 0.0f && pct < 90.0f);
    });
    std::stable_sort(candidates.begin(), candidates.end(), [&](Candidate const& left, Candidate const& right)
    {
        return health(left) < health(right);
    });
    for (Candidate const& candidate : candidates)
        if (attempt(candidate))
            return true;
    return false;
}

void LogKnownAbilities(Player const& bot);
std::vector<Player*> HealCandidates(Player& bot, Player* owner);
bool HealParty(Player& bot, Player* owner);
bool MaintainBuff(Player& bot, Player& owner);
}

#endif
