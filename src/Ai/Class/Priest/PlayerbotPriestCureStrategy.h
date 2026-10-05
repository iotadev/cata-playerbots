/*
 * Adapted from donor PriestCureStrategy / CureDiseaseTrigger at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_PRIEST_CURE_STRATEGY_H
#define PLAYERBOT_PRIEST_CURE_STRATEGY_H
#include "../../../Bot/Engine/Strategy/Strategy.h"
#include "../../Base/PlayerbotPartySupport.h"

namespace PlayerbotPriestCure
{
inline bool CanCure(bool enabled, bool alive, bool learned, bool nativeEligible)
{
    return PlayerbotPartySupport::CanCure(enabled, alive, learned, nativeEligible);
}
class CureStrategy final : public Strategy
{
public:
    explicit CureStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "cure"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_NONCOMBAT; } // Utility, not a healer role.
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        // Cata has no Abolish Disease: wire the donor's real Cure fallback.
        triggers.push_back(new TriggerNode("cure disease", { NextAction("cure disease", 31.0f) }));
        triggers.push_back(new TriggerNode("party member cure disease", { NextAction("cure disease on party", 30.0f) }));
    }
};
}
#endif
