/*
 * Cata adaptation of mod-playerbots BuffOnPartyTrigger / party buff actions.
 * Donor 7bae1b5c58c76a0aa20381155edc08096d1485b2; see PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotPartyBuffStrategy.h"
#include "../../Bot/PlayerbotAI.h"
#include "../../Bot/Engine/Value/Value.h"
#include "../../Script/PlayerbotConfig.h"
#include "Player.h"
#include "SpellMgr.h"
#include "SpellHistory.h"
#include "Timer.h"

namespace
{
class PartyMemberWithoutAuraValue final : public CalculatedValue<ObjectGuid>, public Qualified
{
public:
    explicit PartyMemberWithoutAuraValue(PlayerbotAI* ai) : CalculatedValue(ai, "party member without aura") { }
private:
    ObjectGuid Calculate() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        Player* owner = botAI ? botAI->GetController() : nullptr;
        auto* buff = bot ? PlayerbotPartyBuff::ResolveAuraBuff(bot->getClass(), qualifier) : nullptr;
        return bot && owner && buff ? PlayerbotDecision::PartyBuffTarget(*bot, *owner, *buff, &botAI->Rebuff()) : ObjectGuid::Empty;
    }
};
bool Needed(PlayerbotAI* ai, PlayerbotDecision::PartyBuff const& buff)
{
    if (!ai || !PlayerbotModuleEnginePartyBuffEnabled())
        return false;
    Player* bot = ai->GetBot();
    Player* owner = ai->GetController();
    AiObjectContext* context = ai->GetAiObjectContext();
    char const* qualifier = PlayerbotPartyBuff::AuraQualifier(buff);
    Value<ObjectGuid>* value = context && qualifier ? context->GetValue<ObjectGuid>("party member without aura", qualifier) : nullptr;
    bool needed = bot && owner && value && !value->Get().IsEmpty();
    if (needed && ai->Rebuff().IsPending(getMSTime()) && !bot->IsInCombat()) ai->Rebuff().NoteWork();
    return needed;
}

class PartyBuffNeededValue final : public CalculatedValue<bool>
{
public:
    PartyBuffNeededValue(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff)
        : CalculatedValue<bool>(ai, name, 2), buff(buff) { }
private:
    bool Calculate() override { return Needed(botAI, buff); }
    PlayerbotDecision::PartyBuff buff;
};

class PartyBuffTrigger final : public Trigger
{
public:
    PartyBuffTrigger(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff)
        : Trigger(ai, name, 2), needed(ai, name, buff) { }
    bool IsActive() override
    {
        if (botAI && botAI->Rebuff().IsPending(getMSTime())) needed.Reset();
        return needed.Get();
    }
    bool IsBuffTrigger() override { return true; }
    void Reset() override { Trigger::Reset(); needed.Reset(); }
private:
    PartyBuffNeededValue needed;
};

class PartyBuffAction final : public Action
{
public:
    PartyBuffAction(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff)
        : Action(ai, name), buff(buff) { }
    bool isUseful() override { return Needed(botAI, buff); }
    bool Execute([[maybe_unused]] Event event) override
    {
        // Resolve again on this map update; never retain players across ticks.
        if (!botAI || !PlayerbotModuleEnginePartyBuffEnabled())
            return false;
        Player* bot = botAI->GetBot();
        Player* owner = botAI->GetController();
        if (!bot || !owner) return false;
        if (botAI->Rebuff().IsPending(getMSTime()) && !bot->IsInCombat()) botAI->Rebuff().NoteProposed();
        bool cast = PlayerbotDecision::MaintainPartyBuff(*bot, *owner, buff, &botAI->Rebuff());
        if (cast) botAI->Rebuff().NoteCast(buff.SpellId);
        return cast;
    }
private:
    PlayerbotDecision::PartyBuff buff;
};
}

Action* PlayerbotPartyBuff::CreateAction(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff)
{
    return new PartyBuffAction(ai, name, buff);
}

Trigger* PlayerbotPartyBuff::CreateTrigger(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff)
{
    return new PartyBuffTrigger(ai, name, buff);
}

void PlayerbotPartyBuff::AddValues(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["party member without aura"] = [](PlayerbotAI* ai) { return new PartyMemberWithoutAuraValue(ai); };
    values.Add(factory);
}

bool PlayerbotPartyBuff::RebuffOnGlobalCooldown(PlayerbotAI& ai)
{
    Player* bot = ai.GetBot();
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(ai.Rebuff().LastSpell());
    TriggerCastFlags flags = TRIGGERED_NONE;
    if (bot && spell) spell = bot->GetCastSpellInfo(spell, flags);
    return bot && spell && bot->GetSpellHistory()->HasGlobalCooldown(spell);
}
bool PlayerbotPartyBuff::WaitForRebuff(PlayerbotAI& ai)
{
    Player* bot = ai.GetBot();
    return bot && !bot->IsInCombat() && ai.Rebuff().IsPending(getMSTime()) &&
        (ai.Rebuff().HasWork() || RebuffOnGlobalCooldown(ai));
}
