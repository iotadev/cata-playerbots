/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 */

#ifndef PLAYERBOT_MAGE_STRATEGY_H
#define PLAYERBOT_MAGE_STRATEGY_H

class Creature;
class Player;

namespace PlayerbotMage
{
void LogKnownAbilities(Player const& bot);
bool Execute(Player& bot, Creature& target);
bool MaintainBuff(Player& bot, Player& owner);
}

#endif
