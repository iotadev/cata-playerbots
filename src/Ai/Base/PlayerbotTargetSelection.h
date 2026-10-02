/* Adapted from donor TargetValue / DpsTargetValue / RtiTargetValue at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_TARGET_SELECTION_H
#define PLAYERBOT_TARGET_SELECTION_H
#include "../../Bot/Engine/AiObjectContext.h"
#include "SharedDefines.h"
#include "Player.h"
#include <cmath>
#include <string_view>

class Engine;
namespace PlayerbotTargetSelection
{
inline char const* FallbackValue(std::uint8_t playerClass, std::uint32_t talentTree,
    bool warriorEnabled, bool mageEnabled, bool priestEnabled)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            return warriorEnabled ? (talentTree == TALENT_TREE_WARRIOR_PROTECTION ? "tank target" : "dps target") : nullptr;
        case CLASS_MAGE: return mageEnabled ? "dps target" : nullptr;
        case CLASS_PRIEST: return priestEnabled ? "dps target" : nullptr;
        default: return nullptr;
    }
}
inline int IconIndex(std::string_view name)
{
    constexpr std::string_view icons[] = {"star", "circle", "diamond", "triangle", "moon", "square", "cross", "skull"};
    for (int i = 0; i < 8; ++i) if (name == icons[i]) return i;
    return -1;
}
struct Candidate
{
    float Distance;
    float Lifetime;
    bool InRange;
    bool Current;
    bool HighPriority;
};
inline int LifetimeBand(Candidate const& value)
{
    int band = value.InRange ? 10 : 0;
    return band + (value.Lifetime >= 5.0f && value.Lifetime <= 30.0f ? 2 : (value.Lifetime > 30.0f ? 0 : 1));
}
inline bool Better(Candidate const& next, Candidate const& old, bool casterRanking)
{
    if (old.HighPriority) return false; // first donor high-priority candidate wins
    if (next.HighPriority) return true;
    if (!casterRanking)
    {
        if (next.InRange != old.InRange) return next.InRange;
        return next.InRange ? next.Lifetime < old.Lifetime : next.Distance < old.Distance;
    }
    int nextBand = LifetimeBand(next), oldBand = LifetimeBand(old);
    if (nextBand != oldBand) return nextBand > oldBand;
    if (nextBand % 10 == 2 || nextBand % 10 == 0) return next.Lifetime < old.Lifetime;
    if (next.Current) return true;
    if (old.Current) return false;
    return next.Lifetime > old.Lifetime;
}
inline bool ValidEstimate(float value) { return std::isfinite(value) && value > 0.0f; }
struct TankCandidate
{
    float Distance;
    float Threat;
    bool HasAggro;
    bool InMelee;
};
inline bool HasTankAggro(bool hasVictim, bool victimIsBot, bool victimIsOtherTank)
{
    return !hasVictim || victimIsBot || victimIsOtherTank;
}
inline int TankBand(TankCandidate const& candidate)
{
    return !candidate.HasAggro ? 2 : (candidate.InMelee ? 1 : 0);
}
inline bool BetterTank(TankCandidate const& next, TankCandidate const& old)
{
    int nextBand = TankBand(next), oldBand = TankBand(old);
    if (nextBand != oldBand) return nextBand > oldBand;
    return nextBand == 2 ? next.Distance < old.Distance : next.Threat < old.Threat;
}
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
// Native map-thread adapter: GUID result, no autonomous pull or retained Unit pointer.
ObjectGuid SelectDpsTarget(PlayerbotAI& ai, Engine const& engine);
ObjectGuid SelectTankTarget(PlayerbotAI& ai, Engine const& engine);
}
#endif
