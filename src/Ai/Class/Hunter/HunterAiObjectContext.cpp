/* Bounded Cata adaptation of mod-playerbots Hunter contexts and Generic,
 * Marksmanship, BeastMastery, Survival and HunterPet strategies at
 * 79bd428115c8f33b74b13de23b133d49c3f75c60. Pet control adapts native Cata
 * PetHandler::HandlePetActionHelper / PetAI, not world-thread packet dispatch.
 * GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "HunterAiObjectContext.h"
#include "PlayerbotHunterPolicy.h"
#include "../../Base/PlayerbotCombatValues.h"
#include "../../Base/PlayerbotCombatMovement.h"
#include "../../Base/PlayerbotCombatDecision.h"
#include "../../Base/PlayerbotTargetSelection.h"
#include "../../Base/PlayerbotPosition.h"
#include "../../Base/PlayerbotThreatStrategy.h"
#include "../../Base/PlayerbotRestStrategy.h"
#include "../../Base/PlayerbotPotionStrategy.h"
#include "../../Base/PlayerbotCorpseLoot.h"
#include "../../../Bot/PlayerbotAI.h"
#include "../../../Script/PlayerbotConfig.h"
#include "Creature.h"
#include "Player.h"
#include "Pet.h"
#include "PetAI.h"
#include "Spell.h"
#include "MotionMaster.h"
#include "Unit.h"
#include "CharmInfo.h"

namespace
{
bool ActorReady(Player* bot)
{
    return bot && bot->getClass() == CLASS_HUNTER && bot->IsInWorld() && bot->IsAlive() &&
        !bot->IsBeingTeleported() && !bot->IsMounted() && !bot->IsCharmed();
}
Pet* OwnedPet(Player& bot)
{
    Pet* pet = bot.GetPet();
    return pet && pet->getPetType() == HUNTER_PET && pet->GetOwnerGUID() == bot.GetGUID() &&
        pet->IsInWorld() && pet->GetMap() == bot.GetMap() && !pet->IsCharmed() && pet->GetCharmInfo() ? pet : nullptr;
}
bool CombatReady(PlayerbotAI* ai)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Creature* target = ai ? ai->GetCurrentTarget() : nullptr;
    return PlayerbotModuleEngineHunterCombatEnabled() && ActorReady(bot) && target && target->IsAlive() &&
        target->IsInWorld() && target->GetMap() == bot->GetMap() && bot->GetVictim() == target &&
        !target->IsControlledByPlayer() && !PlayerbotTargetSelection::IsProtectedTarget(*target) &&
        bot->IsValidAttackTarget(target) && bot->IsWithinLOSInMap(target) &&
        bot->IsWithinDistInMap(target, 35.0f) && bot->HasInArc(2.0f * float(M_PI) / 3.0f, target) &&
        !bot->IsNonMeleeSpellCast(false, false, true);
}
enum class Intent { Auto, Sting, Builder, Dump, KillShot, Special, KillCommand };
bool ShotReady(PlayerbotAI* ai, uint32 spell, Intent intent)
{
    if (!CombatReady(ai)) return false;
    Player* bot = ai->GetBot();
    Creature* target = ai->GetCurrentTarget();
    if (!bot->HasSpell(spell)) return false;
    if (intent == Intent::Auto)
    {
        Spell* repeat = bot->GetCurrentSpell(CURRENT_AUTOREPEAT_SPELL);
        return !repeat || repeat->m_targets.GetUnitTargetGUID() != target->GetGUID();
    }
    if (intent == Intent::Sting) return !target->HasAura(1978, bot->GetGUID());
    if (intent == Intent::Builder) return PlayerbotHunter::FocusBuilder(bot->GetPower(POWER_FOCUS));
    if (intent == Intent::Dump) return PlayerbotHunter::FocusDump(bot->GetPower(POWER_FOCUS));
    if (intent == Intent::KillShot) return PlayerbotHunter::KillShot(target->GetHealthPct());
    if (intent == Intent::KillCommand)
    {
        Pet* pet = OwnedPet(*bot);
        return pet && pet->IsAlive() && pet->GetVictim() == target;
    }
    return spell != 53301 || !target->HasAura(53301, bot->GetGUID());
}
class ShotAction final : public Action
{
public:
    ShotAction(PlayerbotAI* ai, std::string const& name, uint32 spell, Intent intent)
        : Action(ai, name), spell(spell), intent(intent) { }
    bool isUseful() override { return ShotReady(botAI, spell, intent); }
    ActionThreatType getThreatType() override { return ActionThreatType::Single; }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!isUseful()) return false;
        // Auto Shot is maintained by native autorepeat updates, never per-tick damage.
        if (intent == Intent::Auto) botAI->GetBot()->InterruptSpell(CURRENT_AUTOREPEAT_SPELL);
        Unit& target = intent == Intent::KillCommand ? static_cast<Unit&>(*botAI->GetBot()) : static_cast<Unit&>(*botAI->GetCurrentTarget());
        return PlayerbotDecision::TryCast(*botAI->GetBot(), target, spell, name.c_str());
    }
private:
    uint32 spell; Intent intent;
};
class ShotTrigger final : public Trigger
{
public:
    ShotTrigger(PlayerbotAI* ai, std::string const& name, uint32 spell, Intent intent)
        : Trigger(ai, name, 1), spell(spell), intent(intent) { }
    bool IsActive() override { return ShotReady(botAI, spell, intent); }
private:
    uint32 spell; Intent intent;
};
enum class PetIntent { Call, Revive, Mend };
uint32 PetSpell(PlayerbotAI* ai, PetIntent intent)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Player* controller = ai ? ai->GetController() : nullptr;
    if (!PlayerbotModuleEngineHunterCombatEnabled() || !ActorReady(bot) || !controller || !controller->IsAlive() ||
        !controller->IsInWorld() || controller->IsBeingTeleported() ||
        controller->GetMap() != bot->GetMap() || bot->IsNonMeleeSpellCast(false, false, true)) return 0;
    Pet* pet = OwnedPet(*bot);
    if (intent == PetIntent::Mend)
        return pet && pet->IsAlive() && PlayerbotHunter::MendPet(pet->GetHealthPct(), pet->HasAura(136, bot->GetGUID())) &&
            bot->HasSpell(136) ? 136 : 0;
    // Summon/revive only the existing active native pet, while the party is idle.
    // No stable scanning, pet replacement, taming, health edits or DB provisioning.
    if (bot->GetPet() && !pet) return 0;
    if (bot->IsInCombat() || controller->IsInCombat() || ai->GetCurrentTarget()) return 0;
    if (intent == PetIntent::Revive)
    {
        PlayerPetData* data = bot->GetPlayerPetDataCurrent();
        bool dead = pet ? !pet->IsAlive() : data && data->Type == HUNTER_PET && !data->SavedHealth;
        return dead && bot->HasSpell(982) ? 982 : 0;
    }
    PlayerPetData* data = bot->GetPlayerPetDataCurrent();
    uint32 spell = !bot->GetPet() && data && data->Type == HUNTER_PET && data->SavedHealth ?
        PlayerbotHunter::CallSpell(data->Slot) : 0;
    return spell && bot->HasSpell(spell) ? spell : 0;
}
class PetAction final : public Action
{
public:
    PetAction(PlayerbotAI* ai, std::string const& name, PetIntent intent) : Action(ai, name), intent(intent) { }
    bool isUseful() override { return PetSpell(botAI, intent) != 0; }
    bool Execute([[maybe_unused]] Event event) override
    {
        uint32 spell = PetSpell(botAI, intent);
        if (!spell) return false;
        Player* bot = botAI->GetBot();
        Unit* target = intent == PetIntent::Mend ? static_cast<Unit*>(OwnedPet(*bot)) : static_cast<Unit*>(bot);
        return target && PlayerbotDecision::TryCast(*bot, *target, spell, name.c_str());
    }
private:
    PetIntent intent;
};
class PetTrigger final : public Trigger
{
public:
    PetTrigger(PlayerbotAI* ai, std::string const& name, PetIntent intent) : Trigger(ai, name, 2), intent(intent) { }
    bool IsActive() override { return PetSpell(botAI, intent) != 0; }
private:
    PetIntent intent;
};
void PetTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("call pet", {NextAction("call pet", ACTION_NORMAL + 2)}));
    triggers.push_back(new TriggerNode("revive pet", {NextAction("revive pet", ACTION_NORMAL + 2)}));
    triggers.push_back(new TriggerNode("mend pet", {NextAction("mend pet", ACTION_HIGH + 6)}));
}
class PetStrategy final : public Strategy
{
public:
    explicit PetStrategy(PlayerbotAI* ai) : Strategy(ai) { }
    std::string const getName() override { return "pet"; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override { PetTriggers(triggers); }
};
class HunterStrategy final : public Strategy
{
public:
    HunterStrategy(PlayerbotAI* ai, std::string route, uint32 tree) : Strategy(ai), route(std::move(route)), tree(tree) { }
    std::string const getName() override { return route; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED | STRATEGY_TYPE_DPS; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        PlayerbotCombatMovement::AddTriggers(triggers, true);
        PetTriggers(triggers);
        auto add = [&](char const* name, float priority) { triggers.push_back(new TriggerNode(name, {NextAction(name, priority)})); };
        add("auto shot", ACTION_HIGH + 10);
        add("kill shot", ACTION_HIGH + 5);
        add("serpent sting", ACTION_HIGH + 2);
        if (tree == TALENT_TREE_HUNTER_MARKSMANSHIP) add("chimera shot", ACTION_HIGH + 4);
        if (tree == TALENT_TREE_HUNTER_SURVIVAL) add("explosive shot", ACTION_HIGH + 4);
        if (tree == TALENT_TREE_HUNTER_BEAST_MASTERY) add("kill command", ACTION_HIGH + 4);
        add("arcane shot", ACTION_NORMAL + 3);
        // Cobra replaces Steady only where learned; native spells generate focus.
        bool marksman = tree == TALENT_TREE_HUNTER_MARKSMANSHIP;
        add("cobra shot", ACTION_NORMAL + (marksman ? 1 : 2));
        add("steady shot", ACTION_NORMAL + (marksman ? 2 : 1));
    }
private:
    std::string route; uint32 tree;
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
        auto* combat = new NamedObjectContext<Strategy>(false, true);
        combat->creators["hunter"] = [](PlayerbotAI* ai) { return new HunterStrategy(ai, "hunter", 0); };
        combat->creators["beast mastery"] = [](PlayerbotAI* ai) { return new HunterStrategy(ai, "beast mastery", TALENT_TREE_HUNTER_BEAST_MASTERY); };
        combat->creators["marksmanship"] = [](PlayerbotAI* ai) { return new HunterStrategy(ai, "marksmanship", TALENT_TREE_HUNTER_MARKSMANSHIP); };
        combat->creators["survival"] = [](PlayerbotAI* ai) { return new HunterStrategy(ai, "survival", TALENT_TREE_HUNTER_SURVIVAL); };
        strategies.Add(combat);
        auto* nc = new NamedObjectContext<Strategy>();
        nc->creators["pet"] = [](PlayerbotAI* ai) { return new PetStrategy(ai); };
        strategies.Add(nc);
        auto* af = new NamedObjectContext<Action>();
        auto* tf = new NamedObjectContext<Trigger>();
        auto shot = [&](char const* name, uint32 spell, Intent intent)
        {
            af->creators[name] = [=](PlayerbotAI* ai) { return new ShotAction(ai, name, spell, intent); };
            tf->creators[name] = [=](PlayerbotAI* ai) { return new ShotTrigger(ai, name, spell, intent); };
        };
        shot("auto shot", 75, Intent::Auto);
        shot("serpent sting", 1978, Intent::Sting);
        shot("steady shot", 56641, Intent::Builder);
        shot("cobra shot", 77767, Intent::Builder);
        shot("arcane shot", 3044, Intent::Dump);
        shot("kill shot", 53351, Intent::KillShot);
        shot("chimera shot", 53209, Intent::Special);
        shot("explosive shot", 53301, Intent::Special);
        shot("kill command", 34026, Intent::KillCommand);
        auto pet = [&](char const* name, PetIntent intent)
        {
            af->creators[name] = [=](PlayerbotAI* ai) { return new PetAction(ai, name, intent); };
            tf->creators[name] = [=](PlayerbotAI* ai) { return new PetTrigger(ai, name, intent); };
        };
        pet("call pet", PetIntent::Call); pet("revive pet", PetIntent::Revive); pet("mend pet", PetIntent::Mend);
        actions.Add(af); triggers.Add(tf);
    }
};
Contexts& Shared() { static Contexts contexts; return contexts; }
}
HunterAiObjectContext::HunterAiObjectContext(PlayerbotAI* ai)
    : AiObjectContext(ai, Shared().strategies, Shared().actions, Shared().triggers, Shared().values) { }

void PlayerbotHunter::UpdatePet(PlayerbotAI& ai, bool attacking)
{
    Player* bot = ai.GetBot();
    if (!bot || bot->getClass() != CLASS_HUNTER || !bot->IsInWorld() || bot->IsBeingTeleported() || bot->IsCharmed()) return;
    Pet* pet = OwnedPet(*bot);
    if (!pet || !pet->IsAlive() || !pet->IsAIEnabled()) return;
    auto* nativeAI = dynamic_cast<PetAI*>(pet->AI());
    if (!nativeAI) return; // Do not replace another script's AI.
    Player* controller = ai.GetController();
    Creature* target = ai.GetCurrentTarget();
    bool actorReady = ActorReady(bot) && !bot->IsCharmed();
    bool controllerReady = controller && controller->IsInWorld() && controller->IsAlive() && !controller->IsBeingTeleported() &&
        controller->GetMap() == bot->GetMap() && bot->IsWithinDistInMap(controller, 35.0f);
    bool validTarget = target && target->IsAlive() && target->GetMap() == bot->GetMap() &&
        !target->IsControlledByPlayer() && bot->IsValidAttackTarget(target) && pet->IsValidAttackTarget(target) &&
        bot->IsWithinDistInMap(target, 35.0f) && pet->IsWithinLOSInMap(target);
    bool attack = PetAttackAllowed(PlayerbotModuleEngineHunterCombatEnabled(), actorReady, controllerReady,
        attacking && target && bot->GetVictim() == target, validTarget,
        target && PlayerbotTargetSelection::IsProtectedTarget(*target), bot->HasAuraType(SPELL_AURA_MOD_PACIFY));
    CharmInfo* info = pet->GetCharmInfo();
    // Controller-led policy: native passive pets attack only on a command.
    pet->SetReactState(REACT_PASSIVE);
    if (attack)
    {
        if (pet->GetVictim() == target && info->IsCommandAttack()) return;
        pet->AttackStop();
        pet->InterruptNonMeleeSpells(false);
        pet->ClearUnitState(UNIT_STATE_FOLLOW);
        info->SetIsCommandAttack(true); info->SetIsAtStay(false); info->SetIsFollowing(false);
        info->SetIsCommandFollow(false); info->SetIsReturning(false);
        nativeAI->_AttackStart(target); // Native PetAI owns attack and chase admission.
        return;
    }
    if (!pet->GetVictim() && !info->IsCommandAttack() && info->HasCommandState(COMMAND_FOLLOW) &&
        (info->IsFollowing() || info->IsReturning())) return;
    pet->AttackStop(); pet->InterruptNonMeleeSpells(false);
    info->SetCommandState(COMMAND_FOLLOW); info->SetIsCommandAttack(false); info->SetIsAtStay(false);
    info->SetIsCommandFollow(true); info->SetIsFollowing(false); info->SetIsReturning(true);
    pet->FollowTarget(bot);
}
