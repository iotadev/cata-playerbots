/*
 * Cata adaptation of mod-playerbots BuffOnPartyTrigger / party buff actions.
 * Donor 7bae1b5c58c76a0aa20381155edc08096d1485b2; see PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "PlayerbotPartyBuffStrategy.h"
#include "../../Bot/PlayerbotAI.h"
#include "../../Bot/Engine/Value/Value.h"
#include "../../Script/PlayerbotConfig.h"

namespace
{
bool Needed(PlayerbotAI* ai, PlayerbotDecision::PartyBuff const& buff)
{
    if (!ai || !PlayerbotModuleEnginePartyBuffEnabled())
        return false;
    Player* bot = ai->GetBot();
    Player* owner = ai->GetController();
    return bot && owner && PlayerbotDecision::PartyBuffNeeded(*bot, *owner, buff);
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
    bool IsActive() override { return needed.Get(); }
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
        return bot && owner && PlayerbotDecision::MaintainPartyBuff(*bot, *owner, buff);
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
