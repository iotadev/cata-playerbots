/* Bounded Cata adaptation of mod-playerbots RogueAiObjectContext,
 * CombatRogueStrategy, AssassinationRogueStrategy and Rogue finishing/combo
 * actions at 79bd428115c8f33b74b13de23b133d49c3f75c60. See PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#include "RogueAiObjectContext.h"
#include "PlayerbotRoguePolicy.h"
#include "../../Base/PlayerbotCombatValues.h"
#include "../../Base/PlayerbotCombatMovement.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "../../Base/PlayerbotPosition.h"
#include "../../Base/PlayerbotThreatStrategy.h"
#include "../../Base/PlayerbotInterruptStrategy.h"
#include "../../Base/PlayerbotRestStrategy.h"
#include "../../Base/PlayerbotPotionStrategy.h"
#include "../../Base/PlayerbotCorpseLoot.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../../Script/PlayerbotConfig.h"
#include "Creature.h"
#include "Player.h"
#include <cmath>

namespace
{
enum class Intent { Builder, Backstab, Slice, Rupture, Eviscerate };
bool Ready(PlayerbotAI* ai, Intent intent, uint32 spell)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Creature* target = ai ? ai->GetCurrentTarget() : nullptr;
    if (!PlayerbotModuleEngineRogueCombatEnabled() || !bot || !target ||
        bot->getClass() != CLASS_ROGUE || !bot->IsAlive() || !bot->IsInWorld() ||
        bot->IsBeingTeleported() || !bot->IsInCombat() || !target->IsAlive() ||
        bot->GetMap() != target->GetMap() || bot->GetVictim() != target ||
        !bot->IsValidAttackTarget(target) || !target->IsWithinMeleeRange(bot) ||
        !bot->IsWithinLOSInMap(target) || !bot->HasInArc(2.0f * float(M_PI) / 3.0f, target) ||
        !bot->HasSpell(spell))
        return false;
    uint8 points = PlayerbotRogue::Combo(bot->GetComboTarget() == target->GetGUID(), bot->GetComboPoints());
    if (intent == Intent::Builder || intent == Intent::Backstab)
        return PlayerbotRogue::BuilderReady(bot->GetPower(POWER_ENERGY), points) &&
            (intent != Intent::Backstab || !target->HasInArc(float(M_PI), bot));
    if (intent == Intent::Slice)
        return PlayerbotRogue::SliceReady(points, bot->HasAura(5171));
    auto* context = ai->GetAiObjectContext();
    auto* estimate = context ? context->GetValue<float>("estimated lifetime", "current target") : nullptr;
    float lifetime = estimate ? estimate->Get() : 0.0f;
    return intent == Intent::Rupture ? PlayerbotRogue::RuptureReady(points, lifetime, target->HasAura(1943, bot->GetGUID())) :
        PlayerbotRogue::FinisherReady(points, lifetime);
}
class RogueAction final : public Action
{
public:
    RogueAction(PlayerbotAI* ai, std::string const& name, uint32 spell, Intent intent)
        : Action(ai, name), spell(spell), intent(intent) { }
    bool isUseful() override { return Ready(botAI, intent, spell); }
    std::vector<NextAction> getAlternatives() override
    {
        // Donor action fallback graph, with autoattack already owned by native
        // Attack/Update rather than a second synthetic melee executor.
        if (spell == 1329) return {NextAction("backstab")};
        if (spell == 53) return {NextAction("sinister strike")};
        if (intent == Intent::Rupture) return {NextAction("eviscerate")};
        return {};
    }
    ActionThreatType getThreatType() override
    {
        return intent == Intent::Slice ? ActionThreatType::None : ActionThreatType::Single;
    }
    bool Execute([[maybe_unused]] Event event) override
    {
        // Re-resolve both target and combo owner at execution, not queue time.
        if (!isUseful()) return false;
        Player* bot = botAI->GetBot();
        Unit& target = intent == Intent::Slice ? static_cast<Unit&>(*bot) : static_cast<Unit&>(*botAI->GetCurrentTarget());
        return PlayerbotDecision::TryCast(*bot, target, spell, name.c_str());
    }
private:
    uint32 spell;
    Intent intent;
};
class RogueTrigger final : public Trigger
{
public:
    RogueTrigger(PlayerbotAI* ai, std::string const& name, uint32 spell, Intent intent)
        : Trigger(ai, name, 1), spell(spell), intent(intent) { }
    bool IsActive() override { return Ready(botAI, intent, spell); }
private:
    uint32 spell;
    Intent intent;
};
class RogueStrategy final : public Strategy
{
public:
    RogueStrategy(PlayerbotAI* ai, std::string name, bool assassination)
        : Strategy(ai), route(std::move(name)), assassination(assassination) { }
    std::string const getName() override { return route; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_MELEE | STRATEGY_TYPE_DPS; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        PlayerbotCombatMovement::AddTriggers(triggers);
        PlayerbotInterrupt::AddTrigger(triggers, "kick");
        triggers.push_back(new TriggerNode("slice and dice", {NextAction("slice and dice", ACTION_HIGH + 5)}));
        triggers.push_back(new TriggerNode("rupture", {NextAction("rupture", ACTION_HIGH + 1)}));
        triggers.push_back(new TriggerNode("eviscerate", {NextAction("eviscerate", ACTION_HIGH)}));
        if (assassination)
        {
            triggers.push_back(new TriggerNode("mutilate", {NextAction("mutilate", ACTION_NORMAL + 4)}));
            triggers.push_back(new TriggerNode("backstab", {NextAction("backstab", ACTION_NORMAL + 3.5f)}));
        }
        triggers.push_back(new TriggerNode("sinister strike", {NextAction("sinister strike", ACTION_NORMAL + 3)}));
    }
private:
    std::string route;
    bool assassination;
};
struct Contexts
{
    SharedNamedObjectContextList<Strategy> strategies;
    SharedNamedObjectContextList<Action> actions;
    SharedNamedObjectContextList<Trigger> triggers;
    SharedNamedObjectContextList<UntypedValue> values;
    Contexts()
    {
        PlayerbotCombatValues::AddContexts(values);
        PlayerbotCombatMovement::AddContexts(actions, triggers, values);
        PlayerbotPosition::AddContexts(strategies, actions, triggers, values);
        PlayerbotThreat::AddContexts(strategies);
        PlayerbotRest::AddContexts(strategies, actions, triggers);
        PlayerbotPotion::AddContexts(strategies, actions, triggers);
        PlayerbotCorpseLoot::AddContexts(strategies, actions, triggers);
        PlayerbotInterrupt::AddContexts(actions, triggers, values, "kick", 1766, PlayerbotModuleEngineRogueCombatEnabled);
        auto* combat = new NamedObjectContext<Strategy>(false, true);
        combat->creators["rogue"] = [](PlayerbotAI* ai) { return new RogueStrategy(ai, "rogue", false); };
        combat->creators["combat"] = [](PlayerbotAI* ai) { return new RogueStrategy(ai, "combat", false); };
        combat->creators["assassination"] = [](PlayerbotAI* ai) { return new RogueStrategy(ai, "assassination", true); };
        strategies.Add(combat);
        auto* actionFactory = new NamedObjectContext<Action>();
        auto* triggerFactory = new NamedObjectContext<Trigger>();
        auto add = [&](char const* name, uint32 spell, Intent intent)
        {
            actionFactory->creators[name] = [=](PlayerbotAI* ai) { return new RogueAction(ai, name, spell, intent); };
            triggerFactory->creators[name] = [=](PlayerbotAI* ai) { return new RogueTrigger(ai, name, spell, intent); };
        };
        add("sinister strike", 1752, Intent::Builder);
        add("mutilate", 1329, Intent::Builder);
        add("backstab", 53, Intent::Backstab);
        add("slice and dice", 5171, Intent::Slice);
        add("rupture", 1943, Intent::Rupture);
        add("eviscerate", 2098, Intent::Eviscerate);
        actions.Add(actionFactory);
        triggers.Add(triggerFactory);
    }
};
Contexts& Shared() { static Contexts contexts; return contexts; }
}
RogueAiObjectContext::RogueAiObjectContext(PlayerbotAI* ai)
    : AiObjectContext(ai, Shared().strategies, Shared().actions, Shared().triggers, Shared().values) { }
