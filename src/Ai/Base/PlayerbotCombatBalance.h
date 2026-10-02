/* Adapted from donor AttackerCountValues.cpp at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_COMBAT_BALANCE_H
#define PLAYERBOT_COMBAT_BALANCE_H
#include "SharedDefines.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

namespace PlayerbotCombatBalance
{
inline float EnemyWeight(std::uint32_t level, std::uint32_t rank)
{
    switch (rank)
    {
        case CREATURE_ELITE_RARE: return float(level) * 2.0f;
        case CREATURE_ELITE_ELITE:
        case CREATURE_ELITE_RAREELITE: return float(level) * 3.0f;
        case CREATURE_ELITE_WORLDBOSS: return float(level) * 20.0f;
        default: return float(level);
    }
}
inline std::uint8_t Percent(float livingMemberLevels, std::uint32_t rosterSize, float enemyLevels)
{
    if (!std::isfinite(livingMemberLevels) || !std::isfinite(enemyLevels) ||
        livingMemberLevels < 0.0f || enemyLevels < 0.0f) return 0;
    if (enemyLevels == 0.0f) return 100;
    float party = rosterSize ? livingMemberLevels / float(rosterSize) * float(std::min(rosterSize, 10u)) : 0.0f;
    return std::uint8_t(std::clamp(party * 100.0f / enemyLevels, 0.0f, 200.0f));
}
inline float ManaThreshold(std::uint8_t balance)
{
    return balance <= 50 ? 85.0f : (balance <= 100 ? 65.0f : 40.0f);
}
}
#endif
