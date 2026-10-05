/* Adapted from donor PlayerbotAI::IsTank/IsHeal/IsDps/IsRanged at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_ROLES_H
#define PLAYERBOT_ROLES_H
#include "../../Bot/Engine/Strategy/Strategy.h"
#include "Player.h"
namespace PlayerbotRoles
{
inline constexpr uint32 RoleFlags = STRATEGY_TYPE_TANK | STRATEGY_TYPE_HEAL | STRATEGY_TYPE_DPS |
    STRATEGY_TYPE_RANGED | STRATEGY_TYPE_MELEE;
// Cata primary trees, not donor Wrath talent-point counts/presence heuristics.
inline uint32 SpecMask(uint8 playerClass, uint32 tree, uint8 form = FORM_NONE)
{
    constexpr uint32 tank = STRATEGY_TYPE_TANK | STRATEGY_TYPE_MELEE;
    constexpr uint32 heal = STRATEGY_TYPE_HEAL | STRATEGY_TYPE_RANGED;
    constexpr uint32 melee = STRATEGY_TYPE_DPS | STRATEGY_TYPE_MELEE;
    constexpr uint32 ranged = STRATEGY_TYPE_DPS | STRATEGY_TYPE_RANGED;
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            if (tree == TALENT_TREE_WARRIOR_PROTECTION) return tank;
            if (tree == TALENT_TREE_WARRIOR_ARMS || tree == TALENT_TREE_WARRIOR_FURY) return melee;
            return STRATEGY_TYPE_MELEE;
        case CLASS_DEATH_KNIGHT:
            if (tree == TALENT_TREE_DEATH_KNIGHT_BLOOD) return tank;
            if (tree == TALENT_TREE_DEATH_KNIGHT_FROST || tree == TALENT_TREE_DEATH_KNIGHT_UNHOLY) return melee;
            return STRATEGY_TYPE_MELEE;
        case CLASS_PALADIN:
            if (tree == TALENT_TREE_PALADIN_PROTECTION) return tank;
            if (tree == TALENT_TREE_PALADIN_HOLY) return heal;
            if (tree == TALENT_TREE_PALADIN_RETRIBUTION) return melee;
            return 0;
        case CLASS_DRUID:
            if (tree == TALENT_TREE_DRUID_RESTORATION) return heal;
            if (tree == TALENT_TREE_DRUID_BALANCE) return ranged;
            if (tree == TALENT_TREE_DRUID_FERAL_COMBAT) return form == FORM_BEAR ? tank : melee;
            return 0;
        case CLASS_SHAMAN:
            if (tree == TALENT_TREE_SHAMAN_RESTORATION) return heal;
            if (tree == TALENT_TREE_SHAMAN_ELEMENTAL) return ranged;
            if (tree == TALENT_TREE_SHAMAN_ENHANCEMENT) return melee;
            return 0;
        case CLASS_PRIEST:
            if (tree == TALENT_TREE_PRIEST_DISCIPLINE || tree == TALENT_TREE_PRIEST_HOLY) return heal;
            if (tree == TALENT_TREE_PRIEST_SHADOW) return ranged;
            return STRATEGY_TYPE_RANGED;
        case CLASS_MAGE: case CLASS_WARLOCK: case CLASS_HUNTER: return ranged;
        case CLASS_ROGUE: return melee;
        default: return 0;
    }
}
inline uint32 ChooseMask(uint32 strategyMask, uint32 specMask)
{ return strategyMask ? strategyMask & RoleFlags : specMask; }
uint32 Mask(Player const& player, bool bySpec = false);
inline bool IsTank(Player const& player) { return (Mask(player) & STRATEGY_TYPE_TANK) != 0; }
inline bool IsHealer(Player const& player) { return (Mask(player) & STRATEGY_TYPE_HEAL) != 0; }
inline bool IsRanged(Player const& player) { return (Mask(player) & STRATEGY_TYPE_RANGED) != 0; }
}
#endif
