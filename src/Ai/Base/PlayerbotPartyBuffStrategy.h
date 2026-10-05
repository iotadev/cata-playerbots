/*
 * Adapted from mod-playerbots MageBuffStrategy and PriestBuffStrategy at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See AUTHORS.md and PORTING.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_PARTY_BUFF_STRATEGY_H
#define PLAYERBOTS_PARTY_BUFF_STRATEGY_H

#include "PlayerbotCombatDecision.h"
#include "../../Bot/Engine/AiObjectContext.h"
#include <string_view>
#include "SharedDefines.h"

namespace PlayerbotPartyBuff
{
inline constexpr PlayerbotDecision::PartyBuff Brilliance { 1459, 79057, 79058, "Arcane Brilliance" };
inline constexpr PlayerbotDecision::PartyBuff Fortitude { 21562, 79104, 79105, "Power Word: Fortitude" };
inline constexpr char MageAction[] = "arcane intellect on party";
inline constexpr char PriestAction[] = "power word: fortitude on party";
inline char const* AuraQualifier(PlayerbotDecision::PartyBuff const& buff)
{
    if (buff.SpellId == Brilliance.SpellId) return "arcane intellect,arcane brilliance";
    if (buff.SpellId == Fortitude.SpellId) return "power word: fortitude,prayer of fortitude";
    return nullptr;
}
inline PlayerbotDecision::PartyBuff const* ResolveAuraBuff(std::uint8_t playerClass, std::string_view qualifier)
{
    if (playerClass == CLASS_MAGE && (qualifier == "arcane intellect" || qualifier == AuraQualifier(Brilliance)))
        return &Brilliance;
    if (playerClass == CLASS_PRIEST && (qualifier == "power word: fortitude" || qualifier == AuraQualifier(Fortitude)))
        return &Fortitude;
    return nullptr; // No arbitrary spell-name lists or cross-class buff routes.
}
void AddValues(SharedNamedObjectContextList<UntypedValue>& values);
bool RebuffOnGlobalCooldown(PlayerbotAI& ai);
bool WaitForRebuff(PlayerbotAI& ai);

class Strategy final : public ::Strategy
{
public:
    Strategy(PlayerbotAI* ai, char const* actionName, float relevance)
        : ::Strategy(ai), actionName(actionName), relevance(relevance) { }
    std::string const getName() override { return "buff"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_NONCOMBAT; }
    void InitTriggers(std::vector<TriggerNode*>& triggers) override
    {
        triggers.push_back(new TriggerNode(actionName, { NextAction(actionName, relevance) }));
    }
private:
    char const* actionName;
    float relevance;
};

Action* CreateAction(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff);
Trigger* CreateTrigger(PlayerbotAI* ai, char const* name, PlayerbotDecision::PartyBuff buff);
}
#endif
