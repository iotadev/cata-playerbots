/*
 * Adapted from donor MageCureStrategy at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md.
 */
#ifndef PLAYERBOT_MAGE_CURE_STRATEGY_H
#define PLAYERBOT_MAGE_CURE_STRATEGY_H
#include "../../../Bot/Engine/Strategy/Strategy.h"

namespace PlayerbotMageCure
{
class CureStrategy final : public Strategy
{
public:
    explicit CureStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "cure"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("remove curse", { NextAction("remove curse", 41.0f) }));
        triggers.push_back(new TriggerNode("remove curse on party", { NextAction("remove curse on party", 40.0f) }));
    }
};
}
#endif
