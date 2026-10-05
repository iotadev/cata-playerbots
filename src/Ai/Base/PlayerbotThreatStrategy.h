/* Adapted from donor ThreatValues / ThreatStrategy at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_THREAT_STRATEGY_H
#define PLAYERBOT_THREAT_STRATEGY_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include "../../Bot/Engine/Value/Value.h"
#include "../../Bot/Engine/Multiplier.h"
#include <algorithm>
#include <cmath>

namespace PlayerbotThreat
{
inline Action::ActionThreatType ClassifySpellTarget(bool selfTarget, bool positive)
{
    return !selfTarget ? Action::ActionThreatType::Single :
        positive ? Action::ActionThreatType::None : Action::ActionThreatType::Aoe;
}
inline float FocusMultiplierValue(Action::ActionThreatType type, bool healing, bool attackerDebuff)
{
    return attackerDebuff || (type == Action::ActionThreatType::Aoe && !healing) ? 0.0f : 1.0f;
}
class FocusMultiplier final : public Multiplier
{
public:
    explicit FocusMultiplier(PlayerbotAI* ai) : Multiplier(ai, "focus") { }
    float GetValue(Action* action) override
    {
        return action ? FocusMultiplierValue(action->getThreatType(), action->isHealingAction(),
            action->isDebuffOnAttacker()) : 1.0f;
    }
};
class FocusStrategy final : public Strategy
{
public:
    explicit FocusStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "focus"; }
    void InitMultipliers(std::vector<Multiplier*>& multipliers) override
    { multipliers.push_back(new FocusMultiplier(botAI)); }
};
inline uint8_t Percent(float botThreat, float tankThreat, bool hasTank, bool inCombat, bool fleeing)
{
    if (!hasTank || fleeing) return 0;
    if (!std::isfinite(botThreat) || !std::isfinite(tankThreat)) return 100;
    if (tankThreat <= 0.0f)
        return botThreat > 0.0f ? 255 : (inCombat ? 100 : 0);
    // Saturate rather than wrapping uint8, and never divide by a zero tank amount.
    return uint8_t(std::clamp(botThreat * 100.0f / tankThreat, 0.0f, 255.0f));
}
inline float DamageMultiplier(Action::ActionThreatType type, bool grouped, uint8_t percent, bool neglect = false,
    uint8_t aoePercent = 0)
{
    // Donor AoE actions must pass both the attacker maximum and current-target guard.
    if (neglect || !grouped || type == Action::ActionThreatType::None) return 1.0f;
    return percent >= 80 || (type == Action::ActionThreatType::Aoe && aoePercent >= 50) ? 0.0f : 1.0f;
}
template <class Range, class Resolve>
uint8_t MaximumPercent(Range const& attackers, Resolve resolve)
{
    uint8_t maximum = 0;
    for (auto const& attacker : attackers)
        maximum = std::max(maximum, resolve(attacker));
    return maximum;
}
class NeglectThreatResetValue final : public ManualSetValue<bool>
{
public:
    explicit NeglectThreatResetValue(PlayerbotAI* ai) : ManualSetValue(ai, false, "neglect threat") { }
    bool Get() override { bool result = value; Reset(); return result; }
};
void AddContexts(SharedNamedObjectContextList<Strategy>& strategies);
}
#endif
