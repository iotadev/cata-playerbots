/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_COMBAT_DECISION_H
#define PLAYERBOT_COMBAT_DECISION_H

#include <cstdint>
#include <span>

class Creature;
class Player;
class Unit;

// The ordered trigger/action pattern is adapted from mod-playerbots. Cata's
// own SpellInfo and Spell machinery remain the authority for every cast.
namespace PlayerbotDecision
{
enum class ActionTarget
{
    Enemy,
    Self
};

struct Action
{
    std::uint32_t SpellId;
    char const* Name;
    bool (*Triggered)(Player const&, Creature const&);
    ActionTarget Target;
};

// Keep priority/fallback deterministic and independent of spell machinery.
// A failed attempt allows the next eligible action to be considered.
template <typename Eligible, typename Attempt>
bool ExecuteByPriority(std::span<Action const> actions, Eligible&& eligible, Attempt&& attempt)
{
    for (Action const& action : actions)
        if (eligible(action) && attempt(action))
            return true;

    return false;
}

bool ExecuteFirstAvailable(Player& bot, Creature& target, std::span<Action const> actions);
bool TryCast(Player& bot, Unit& target, std::uint32_t spellId, char const* name);

struct PartyBuff
{
    std::uint32_t SpellId;
    std::uint32_t SingleAuraId;
    std::uint32_t PartyAuraId;
    char const* Name;
};

inline bool NeedsPartyBuff(bool learned, bool singleAuraPresent, bool partyAuraPresent)
{
    return learned && !singleAuraPresent && !partyAuraPresent;
}

bool MaintainPartyBuff(Player& bot, Player& owner, PartyBuff const& buff);
bool PartyBuffNeeded(Player& bot, Player& owner, PartyBuff const& buff);
}

#endif
