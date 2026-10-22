/* Adapted from mod-playerbots Rogue strategies, CastComboAction and combo/energy
 * triggers at 79bd428115c8f33b74b13de23b133d49c3f75c60. See PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_ROGUE_POLICY_H
#define PLAYERBOT_ROGUE_POLICY_H
#include <cstdint>
#include <cmath>
namespace PlayerbotRogue
{
inline std::uint8_t Combo(bool onCurrentTarget, std::uint8_t points)
{
    return onCurrentTarget && points <= 5 ? points : 0;
}
inline bool BuilderReady(std::uint32_t energy, std::uint8_t points)
{
    // Donor high-energy trigger is scheduling policy, not a replacement for
    // native cost/weapon/GCD checks. Never spend a builder at the combo cap.
    return energy >= 60 && points < 5;
}
inline bool FinisherReady(std::uint8_t points, float lifetime)
{
    // Donor TriggerContext binds the near-death trigger to 3 points / 3 sec,
    // rather than the generic trigger constructor's 5 points / 8 sec defaults.
    return points == 5 || (points >= 3 && points <= 5 &&
        std::isfinite(lifetime) && lifetime > 0.0f && lifetime <= 3.0f);
}
inline bool SliceReady(std::uint8_t points, bool present)
{
    return points > 0 && points <= 5 && !present;
}
inline bool RuptureReady(std::uint8_t points, float lifetime, bool ownAura)
{
    // Near-death points are spent on immediate damage, not a fresh bleed.
    return points == 5 && !ownAura &&
        (!std::isfinite(lifetime) || lifetime <= 0.0f || lifetime > 3.0f);
}
}
#endif
