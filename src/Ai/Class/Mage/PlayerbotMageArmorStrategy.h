/*
 * Adapted from mod-playerbots MageBuffManaStrategy / MageBuffDpsStrategy at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md and AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_MAGE_ARMOR_STRATEGY_H
#define PLAYERBOT_MAGE_ARMOR_STRATEGY_H
#include "../../../Bot/Engine/Strategy/Strategy.h"
#include "../../../Bot/Engine/NamedObjectContext.h"
#include <cstdint>

namespace PlayerbotMageArmor
{
inline constexpr std::uint32_t Frost = 7302, Mage = 6117, Molten = 30482;
// Cata policy, not a claim of complete donor specialization parity.
constexpr std::uint32_t Select(bool arcane, bool mageKnown, bool moltenKnown, bool frostKnown)
{
    if (arcane && mageKnown)
        return Mage;
    if (moltenKnown)
        return Molten;
    if (mageKnown)
        return Mage;
    return frostKnown ? Frost : 0;
}
class ArmorStrategy final : public Strategy
{
public:
    ArmorStrategy(PlayerbotAI* ai, bool mana) : Strategy(ai), mana(mana) { }
    std::string const getName() override { return mana ? "bmana" : "bdps"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        if (mana)
        {
            triggers.push_back(new TriggerNode("mage armor", { NextAction("mage armor", 19.0f) }));
            // Cata learned-spell fallback; Wrath's separate Ice Armor is not used.
            triggers.push_back(new TriggerNode("frost armor", { NextAction("frost armor", 19.0f) }));
        }
        else
            triggers.push_back(new TriggerNode("molten armor", { NextAction("molten armor", 19.0f) }));
    }
private:
    bool mana;
};
void AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers);
}
#endif
