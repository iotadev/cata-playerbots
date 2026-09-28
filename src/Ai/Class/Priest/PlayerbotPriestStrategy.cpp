/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#include "PlayerbotPriestStrategy.h"
#include "Group.h"
#include "Log.h"
#include "Player.h"
#include "PlayerbotCombatDecision.h"

void PlayerbotPriest::LogKnownAbilities(Player const& bot)
{
    TC_LOG_INFO("server", "PB-HEAL: Priest level=%u Shield=%u Flash Heal=%u Heal=%u Renew=%u",
        bot.getLevel(), bot.HasSpell(17), bot.HasSpell(2061), bot.HasSpell(2050), bot.HasSpell(139));
}

bool PlayerbotPriest::MaintainBuff(Player& bot, Player& owner)
{
    if (bot.getClass() != CLASS_PRIEST)
        return false;
    // Cata's 21562 dummy cast applies 79104 alone or 79105 to the party.
    constexpr PlayerbotDecision::PartyBuff fortitude { 21562, 79104, 79105, "Power Word: Fortitude" };
    return PlayerbotDecision::MaintainPartyBuff(bot, owner, fortitude);
}

bool PlayerbotPriest::HealParty(Player& bot, Player& owner)
{
    if (bot.getClass() != CLASS_PRIEST || !bot.IsAlive() || !owner.IsAlive() ||
        bot.IsNonMeleeSpellCast(false) || bot.GetMap() != owner.GetMap())
        return false;

    std::vector<Player*> candidates;
    auto consider = [&](Player* member)
    {
        if (!member || !member->IsAlive() || member->GetMap() != bot.GetMap() ||
            (member != &bot && (!bot.IsWithinDistInMap(member, 30.0f) || !bot.IsWithinLOSInMap(member))))
            return;

        // Self and owner also appear in group iteration; try each only once.
        if (std::find(candidates.begin(), candidates.end(), member) == candidates.end())
            candidates.push_back(member);
    };

    consider(&bot);
    consider(&owner);
    Group* group = bot.GetGroup();
    if (group && owner.GetGroup() == group)
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            consider(ref->GetSource());

    // The priority/health-trigger pattern is adapted from Playerbots' Priest
    // heal strategy. All casts still pass through Cata spell and range checks.
    return TryInHealthOrder(candidates, [](Player* member) { return member->GetHealthPct(); }, [&](Player* target)
    {
        float healthPct = target->GetHealthPct();
        if (healthPct < 35.0f && bot.HasSpell(17) &&
            !target->HasAura(17) && !target->HasAura(6788) &&
            PlayerbotDecision::TryCast(bot, *target, 17, "Power Word: Shield"))
            return true;

        switch (TierForHealth(healthPct))
        {
            case HealTier::Emergency:
                if (bot.HasSpell(2061) && PlayerbotDecision::TryCast(bot, *target, 2061, "Flash Heal"))
                    return true;
                [[fallthrough]];
            case HealTier::Heal:
                if (bot.HasSpell(2050) && PlayerbotDecision::TryCast(bot, *target, 2050, "Heal"))
                    return true;
                [[fallthrough]];
            case HealTier::Renew:
                return bot.HasSpell(139) && !target->HasAura(139, bot.GetGUID()) &&
                    PlayerbotDecision::TryCast(bot, *target, 139, "Renew");
            case HealTier::None:
                return false;
        }
        return false;
    });
}
