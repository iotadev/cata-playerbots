/* Adapted from donor PriestHealerDpsStrategy / HealerShouldAttackTrigger at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_PRIEST_DPS_STRATEGY_H
#define PLAYERBOT_PRIEST_DPS_STRATEGY_H
#include "../../../Bot/Engine/Strategy/Strategy.h"
#include "../../Base/PlayerbotCombatBalance.h"
#include <array>
#include <cstdint>

namespace PlayerbotPriestDps
{
struct DamageSpell
{
    char const* Name;
    std::uint32_t SpellId;
    float Priority;
};
inline constexpr std::array<DamageSpell, 4> Spells = {{{"shadow word: pain", 589, 5.5f},
    {"holy fire", 14914, 5.4f}, {"smite", 585, 5.3f}, {"mind blast", 8092, 5.2f}}};
inline bool NeedsDamageCast(bool periodicDamage, bool ownAuraPresent)
{
    return !periodicDamage || !ownAuraPresent;
}
inline bool CanAttack(bool enabled, bool learned, bool controlledTarget,
    bool healingNeeded, float manaPct, std::uint8_t balance = 0)
{
    // Missing balance defaults to the donor's most conservative branch.
    return enabled && learned && controlledTarget && !healingNeeded &&
        manaPct >= PlayerbotCombatBalance::ManaThreshold(balance);
}
class HealerDpsStrategy final : public Strategy
{
public:
    explicit HealerDpsStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "healer dps"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_HEAL; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        std::vector<NextAction> actions;
        for (DamageSpell const& spell : Spells)
            actions.emplace_back(spell.Name, spell.Priority);
        triggers.push_back(new TriggerNode("healer should attack", actions));
    }
};
}
#endif
