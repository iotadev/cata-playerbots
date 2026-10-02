/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotCombatDecision.h"
#include "PlayerbotPartySupport.h"
#include "Creature.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "Spell.h"
#include "SpellHistory.h"
#include "SpellMgr.h"

bool PlayerbotDecision::TryCast(Player& bot, Unit& target, std::uint32_t spellId, char const* name)
{
    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId);
    if (!spellInfo || !bot.HasSpell(spellId))
        return false;
    // Match native pending casts: authorize the learned base spell first,
    // then resolve any current action-bar override and native cost flags.
    TriggerCastFlags triggerFlags = TRIGGERED_NONE;
    spellInfo = bot.GetCastSpellInfo(spellInfo, triggerFlags);
    if (!spellInfo || bot.GetSpellHistory()->HasGlobalCooldown(spellInfo) ||
        bot.GetSpellHistory()->HasCooldown(spellInfo) || !bot.CanRequestSpellCast(spellInfo))
        return false;

    Spell* spell = new Spell(&bot, spellInfo, triggerFlags);
    if (!spell->CanAutoCast(&target))
    {
        delete spell;
        return false;
    }

    SpellCastTargets targets;
    targets.SetUnitTarget(&target);
    std::string targetName = target.GetName();
    if (spell->prepare(targets) == SPELL_CAST_OK)
    {
        TC_LOG_INFO("server", "PB-02: %s began %s on %s", bot.GetName().c_str(), name, targetName.c_str());
        return true;
    }
    return false;
}

bool PlayerbotDecision::ExecuteFirstAvailable(Player& bot, Creature& target, std::span<Action const> actions)
{
    auto eligible = [&bot, &target](Action const& action)
    {
        return action.Triggered(bot, target) && bot.HasSpell(action.SpellId);
    };
    auto attempt = [&bot, &target](Action const& action)
    {
        Unit& castTarget = action.Target == ActionTarget::Self ? static_cast<Unit&>(bot) : static_cast<Unit&>(target);
        return TryCast(bot, castTarget, action.SpellId, action.Name);
    };

    return ExecuteByPriority(actions, eligible, attempt);
}

std::vector<Player*> PlayerbotPartySupport::Candidates(Player& bot, Player* owner)
{
    std::vector<Player*> candidates;
    if (!bot.IsAlive())
        return candidates;
    auto consider = [&](Player* member)
    {
        if (!member || !member->IsAlive() || member->GetMap() != bot.GetMap() ||
            (member != &bot && (!bot.IsWithinDistInMap(member, 30.0f) || !bot.IsWithinLOSInMap(member))))
            return;
        if (std::find(candidates.begin(), candidates.end(), member) == candidates.end())
            candidates.push_back(member);
    };
    consider(&bot);
    consider(owner);
    if (Group* group = bot.GetGroup())
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            consider(ref->GetSource());
    return candidates;
}

bool PlayerbotPartySupport::HasDispellableAura(Player& bot, Player& target, std::uint32_t spellId, std::uint32_t dispelType)
{
    SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId);
    if (!spell || !target.IsAlive() || !bot.IsFriendlyTo(&target) || target.GetMap() != bot.GetMap() ||
        !bot.IsWithinDistInMap(&target, 30.0f) || !bot.IsWithinLOSInMap(&target))
        return false;
    bool matchingDispel = false;
    for (SpellEffectInfo const& effect : spell->Effects)
        if (effect.IsEffect(SPELL_EFFECT_DISPEL) && effect.MiscValue == int32(dispelType))
            matchingDispel = true;
    if (!matchingDispel)
        return false;
    DispelChargesList auras;
    target.GetDispellableAuraList(&bot, SpellInfo::GetDispelMask(DispelType(dispelType)), auras);
    return !auras.empty(); // No native aura pointer survives this call.
}

namespace
{
template <typename Attempt>
bool VisitPartyBuffCandidates(Player& bot, Player& owner, PlayerbotDecision::PartyBuff const& buff, Attempt&& attempt)
{
    if (!bot.IsAlive() || !owner.IsAlive() || bot.IsInCombat() || owner.IsInCombat() ||
        bot.IsNonMeleeSpellCast(false) || !bot.HasSpell(buff.SpellId) || bot.GetMap() != owner.GetMap())
        return false;

    auto apply = [&](Player* member)
    {
        if (!member || !member->IsAlive() || member->IsInCombat() || member->GetMap() != bot.GetMap() ||
            (member != &bot && (!bot.IsWithinDistInMap(member, 30.0f) || !bot.IsWithinLOSInMap(member))))
            return false;
        return PlayerbotDecision::NeedsPartyBuff(true, member->HasAura(buff.SingleAuraId), member->HasAura(buff.PartyAuraId)) &&
            attempt(*member);
    };

    if (apply(&bot) || apply(&owner))
        return true;
    Group* group = bot.GetGroup();
    if (group && owner.GetGroup() == group)
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            if (ref->GetSource() != &bot && ref->GetSource() != &owner && apply(ref->GetSource()))
                return true;
    return false;
}
}

bool PlayerbotDecision::PartyBuffNeeded(Player& bot, Player& owner, PartyBuff const& buff)
{
    return VisitPartyBuffCandidates(bot, owner, buff, [](Player&) { return true; });
}

bool PlayerbotDecision::MaintainPartyBuff(Player& bot, Player& owner, PartyBuff const& buff)
{
    return VisitPartyBuffCandidates(bot, owner, buff, [&](Player& member)
    {
        return TryCast(bot, member, buff.SpellId, buff.Name);
    });
}
