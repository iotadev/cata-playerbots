/*
 * Cata adapter for donor Mage armor strategies. See matching header and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotMageArmorStrategy.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../../Script/PlayerbotConfig.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "Player.h"
#include <utility>

namespace
{
bool Needed(PlayerbotAI* ai, uint32 spell)
{
    if (!ai || !PlayerbotModuleMageArmorEnabled() || ai->GetRestSpellId() || ai->LootRequests().Pending() || ai->LootPursuit().Active())
        return false;
    Player* bot = ai->GetBot();
    Player* owner = ai->GetController();
    if (!bot || !owner || bot->getClass() != CLASS_MAGE || !bot->IsAlive() || !owner->IsAlive() ||
        bot->IsInCombat() || owner->IsInCombat() || bot->IsBeingTeleported() ||
        bot->IsMounted() || bot->IsInFlight() || bot->IsNonMeleeSpellCast(false) ||
        !bot->IsWithinDistInMap(owner, 30.0f))
        return false;
    return PlayerbotMageArmor::Select(
        bot->GetPrimaryTalentTree(bot->GetActiveSpec()) == TALENT_TREE_MAGE_ARCANE,
        bot->HasSpell(PlayerbotMageArmor::Mage), bot->HasSpell(PlayerbotMageArmor::Molten),
        bot->HasSpell(PlayerbotMageArmor::Frost)) == spell && !bot->HasAura(spell);
}
class ArmorTrigger final : public Trigger
{
public:
    ArmorTrigger(PlayerbotAI* ai, char const* name, uint32 spell) : Trigger(ai, name, 2), spell(spell) { }
    bool IsActive() override { return Needed(botAI, spell); }
    bool IsBuffTrigger() override { return true; }
private:
    uint32 spell;
};
class ArmorAction final : public Action
{
public:
    ArmorAction(PlayerbotAI* ai, char const* name, uint32 spell) : Action(ai, name), spell(spell) { }
    bool isUseful() override { return Needed(botAI, spell); }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful())
            return false;
        Player* bot = botAI->GetBot();
        // Native spell exclusivity replaces old armor only if the cast succeeds.
        return PlayerbotDecision::TryCast(*bot, *bot, spell, name.c_str());
    }
private:
    uint32 spell;
};
}
void PlayerbotMageArmor::AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers)
{
    auto* strategyFactory = new NamedObjectContext<Strategy>();
    strategyFactory->creators["bmana"] = [](PlayerbotAI* ai) { return new ArmorStrategy(ai, true); };
    strategyFactory->creators["bdps"] = [](PlayerbotAI* ai) { return new ArmorStrategy(ai, false); };
    strategies.Add(strategyFactory);
    auto* actionFactory = new NamedObjectContext<Action>();
    auto* triggerFactory = new NamedObjectContext<Trigger>();
    for (auto const& [name, spell] : { std::pair{"mage armor", Mage},
        std::pair{"molten armor", Molten}, std::pair{"frost armor", Frost} })
    {
        actionFactory->creators[name] = [name, spell](PlayerbotAI* ai) { return new ArmorAction(ai, name, spell); };
        triggerFactory->creators[name] = [name, spell](PlayerbotAI* ai) { return new ArmorTrigger(ai, name, spell); };
    }
    actions.Add(actionFactory);
    triggers.Add(triggerFactory);
}
