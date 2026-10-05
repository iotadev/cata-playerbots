/* Adapted from donor UsePotionsStrategy / UseHealingPotion / UseManaPotion at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_POTION_STRATEGY_H
#define PLAYERBOT_POTION_STRATEGY_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include <cmath>

class Item;
class SpellInfo;
namespace PlayerbotPotion
{
enum class Kind { Healing, Mana, Healthstone };
inline bool MatchesItem(bool consumable, bool potion, uint32_t firstSpell, Kind kind)
{
    // Cata uses the native 6262 Healthstone heal script, not WotLK rank/name lists.
    return consumable && (kind == Kind::Healthstone ? !potion && firstSpell == 6262 : potion);
}
inline bool PotionLockoutApplies(Kind kind, bool lastPotion)
{
    return kind != Kind::Healthstone && lastPotion;
}
inline bool Needs(Kind kind, float health, float mana, bool usesMana)
{
    if (!std::isfinite(health) || health <= 0.0f) return false;
    return kind != Kind::Mana ? health < 25.0f :
        usesMana && std::isfinite(mana) && mana >= 0.0f && mana < 40.0f;
}
inline bool CanUse(bool enabled, bool alive, bool combat, bool transferring,
    bool casting, bool unavailable, bool lastPotion)
{
    return enabled && alive && combat && !transferring && !casting && !unavailable && !lastPotion;
}
class PotionActionNodeFactory final : public NamedObjectFactory<ActionNode>
{
public:
    PotionActionNodeFactory()
    {
        creators["healthstone"] = [](PlayerbotAI*)
        { return new ActionNode("healthstone", {}, {NextAction("healing potion")}, {}); };
    }
};
class PotionStrategy final : public Strategy
{
public:
    explicit PotionStrategy(PlayerbotAI* ai) : Strategy(ai)
    { actionNodeFactories.Add(new PotionActionNodeFactory()); }
    std::string const getName() override { return "potions"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode("potion critical health", {NextAction("healthstone", ACTION_MEDIUM_HEAL + 1)}));
        triggers.push_back(new TriggerNode("potion medium mana", {NextAction("mana potion", ACTION_EMERGENCY)}));
    }
};
// Shared carried-stock classification. It deliberately does not test current cooldowns or need.
SpellInfo const* RecoverySpell(Item const& item, Kind kind);
void AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers);
}
#endif
