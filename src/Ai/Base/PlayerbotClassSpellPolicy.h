/*
 * Cata adaptation of donor RageAvailable and Frost spell conditions at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_CLASS_SPELL_POLICY_H
#define PLAYERBOT_CLASS_SPELL_POLICY_H
#include <cstdint>
#include <cmath>
namespace PlayerbotClassSpell
{
inline bool SpellstealReady(bool learned, bool nativeCandidate, bool cannotBeStolen)
{
    return learned && nativeCandidate && !cannotBeStolen;
}
inline bool MageDefenseReady(bool learned, bool blocked, float healthPct, float threshold, bool beingAttacked = false)
{
    return learned && !blocked && std::isfinite(healthPct) && healthPct >= 0.0f && healthPct <= 100.0f &&
        (healthPct < threshold || beingAttacked);
}
inline bool BrainFreezeReady(bool learned, bool affectingCastTimeProc, std::int32_t castTimePercent)
{
    return learned && affectingCastTimeProc && castTimePercent <= -100;
}
inline bool DeepFreezeReady(bool learned, bool nativeFrozenState)
{
    return learned && nativeFrozenState;
}
inline bool HotStreakReady(bool learnedBase, bool nativeProc, bool nativeOverride)
{
    return learnedBase && nativeProc && nativeOverride;
}
inline bool CriticalMassScorchReady(bool learned, bool affectingTalent, bool debuffPresent)
{
    return learned && affectingTalent && !debuffPresent;
}
inline bool ArcaneMissilesReady(bool learned, bool nativeProcPresent)
{
    return learned && nativeProcPresent;
}
inline bool ArcaneBlastAtCap(std::uint32_t stacks, std::uint32_t nativeCap)
{
    return nativeCap && stacks >= nativeCap;
}
inline bool ColossusSmashReady(bool learned, bool ownDebuffPresent)
{
    return learned && !ownDebuffPresent;
}
inline bool RagingBlowReady(bool learned, bool nativeEnrageState)
{
    return learned && nativeEnrageState;
}
inline bool OverpowerReady(bool learned, bool reactiveOnCurrentTarget, bool affectingProc)
{
    return learned && (reactiveOnCurrentTarget || affectingProc);
}
inline bool InstantSlamReady(bool learned, bool affectingCastTimeProc, std::int32_t castTimePercent)
{
    return learned && affectingCastTimeProc && castTimePercent <= -100;
}
inline bool ExecuteReady(float targetHealthPct)
{
    return std::isfinite(targetHealthPct) && targetHealthPct >= 0.0f && targetHealthPct < 20.0f;
}
inline bool DefensiveHealthReady(float healthPct, bool critical)
{
    return std::isfinite(healthPct) && healthPct >= 0.0f &&
        healthPct < (critical ? 25.0f : 45.0f);
}
// Donor six-second refresh window, with the native Cata stack cap supplied
// by SpellInfo rather than the Wrath five-stack constant.
inline bool SunderArmorNeeded(bool present, std::uint32_t stacks,
    std::uint32_t nativeStackCap, std::int32_t remainingMs)
{
    return nativeStackCap && (!present || stacks < nativeStackCap ||
        (remainingMs >= 0 && remainingMs <= 6000));
}
inline bool HeroicStrikeReady(bool learned, std::uint32_t nativeRage, bool tank)
{
    return learned && nativeRage >= (tank ? 600u : 400u);
}
inline bool IceLanceReady(bool learned, bool frozen, bool fingersOfFrost)
{
    return learned && (frozen || fingersOfFrost);
}
}
#endif
