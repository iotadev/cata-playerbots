/* Cata adaptation of donor Hunter strategy scheduling at
 * 79bd428115c8f33b74b13de23b133d49c3f75c60. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_HUNTER_POLICY_H
#define PLAYERBOT_HUNTER_POLICY_H
#include <cstdint>
#include <cmath>
namespace PlayerbotHunter
{
inline bool FocusBuilder(std::uint32_t focus) { return focus < 60; }
inline bool FocusDump(std::uint32_t focus) { return focus >= 60; }
inline bool KillShot(float health)
{
    return std::isfinite(health) && health >= 0.0f && health <= 20.0f;
}
inline std::uint32_t CallSpell(std::uint8_t slot)
{
    constexpr std::uint32_t spells[] = {883, 83242, 83243, 83244, 83245};
    return slot < 5 ? spells[slot] : 0;
}
inline bool PetAttackAllowed(bool enabled, bool actorReady, bool controllerReady,
    bool currentVictim, bool validTarget, bool protectedTarget, bool pacified)
{
    return enabled && actorReady && controllerReady && currentVictim && validTarget && !protectedTarget && !pacified;
}
inline bool MendPet(float health, bool present)
{
    return !present && std::isfinite(health) && health > 0.0f && health < 70.0f;
}
}
#endif
