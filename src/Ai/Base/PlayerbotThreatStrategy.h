/* Adapted from donor ThreatValues / ThreatStrategy at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#ifndef PLAYERBOT_THREAT_STRATEGY_H
#define PLAYERBOT_THREAT_STRATEGY_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include "../../Bot/Engine/Value/Value.h"
#include <algorithm>
#include <cmath>

namespace PlayerbotThreat
{
inline uint8_t Percent(float botThreat, float tankThreat, bool hasTank, bool inCombat, bool fleeing)
{
    if (!hasTank || fleeing) return 0;
    if (!std::isfinite(botThreat) || !std::isfinite(tankThreat)) return 100;
    if (tankThreat <= 0.0f)
        return botThreat > 0.0f ? 255 : (inCombat ? 100 : 0);
    // Saturate rather than wrapping uint8, and never divide by a zero tank amount.
    return uint8_t(std::clamp(botThreat * 100.0f / tankThreat, 0.0f, 255.0f));
}
inline float DamageMultiplier(Action::ActionThreatType type, bool grouped, uint8_t percent, bool neglect = false)
{
    // The donor's AoE 50% path awaits the complete attacker-value port.
    return !neglect && grouped && type == Action::ActionThreatType::Single && percent >= 80 ? 0.0f : 1.0f;
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
