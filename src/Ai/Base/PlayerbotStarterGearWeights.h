/* Starter adaptation of donor StatsWeightCalculator::GenerateBasicWeights at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_STARTER_GEAR_WEIGHTS_H
#define PLAYERBOT_STARTER_GEAR_WEIGHTS_H
#include "PlayerbotEquipment.h"
#include "Player.h"
namespace PlayerbotEquipment
{
inline float WeaponMultiplier(uint8 playerClass, uint32 tree, uint32 inventoryType, uint32 subclass,
    bool dualWield, bool titanGrip)
{
    bool twoHand = inventoryType == INVTYPE_2HWEAPON;
    float multiplier = twoHand ? 0.5f : 1.0f;
    if (playerClass == CLASS_WARRIOR)
    {
        bool fury = tree == TALENT_TREE_WARRIOR_FURY;
        if (twoHand && (tree == TALENT_TREE_WARRIOR_PROTECTION || (fury && dualWield && !titanGrip))) multiplier *= 0.1f;
        if (!twoHand && (tree == TALENT_TREE_WARRIOR_ARMS || (fury && !dualWield))) multiplier *= 0.1f;
        if (fury && titanGrip && (!twoHand || subclass == ITEM_SUBCLASS_WEAPON_POLEARM || subclass == ITEM_SUBCLASS_WEAPON_STAFF))
            multiplier *= 0.1f;
    }
    else if (!twoHand && (playerClass == CLASS_MAGE || playerClass == CLASS_PRIEST)) multiplier *= 0.65f;
    return multiplier; // Donor weapon heuristic; native slot admission is separate.
}
inline bool LayoutComparable(uint8 slot, bool candidateTwoHand, bool existingTwoHand, bool hasOffhand)
{
    if (slot == EQUIPMENT_SLOT_OFFHAND && (candidateTwoHand || existingTwoHand)) return false;
    if (slot != EQUIPMENT_SLOT_MAINHAND) return true;
    return candidateTwoHand == existingTwoHand && !(candidateTwoHand && hasOffhand);
}
// Source-adapted starter heuristics, not Cata endgame/BiS weights. The runtime
// reader is separately default-off. No equipment action consumes this model.
inline WeightModel StarterModel(uint8 playerClass, uint32 tree, uint8 level)
{
    using namespace PlayerbotItemStats;
    WeightModel model;
    model.Class = playerClass; model.Spec = tree;
    model.MinimumLevel = 10; model.MaximumLevel = 39;
    if (level < model.MinimumLevel || level > model.MaximumLevel) return model;
    bool warrior = playerClass == CLASS_WARRIOR &&
        (tree == TALENT_TREE_WARRIOR_ARMS || tree == TALENT_TREE_WARRIOR_FURY || tree == TALENT_TREE_WARRIOR_PROTECTION);
    bool mage = playerClass == CLASS_MAGE &&
        (tree == TALENT_TREE_MAGE_ARCANE || tree == TALENT_TREE_MAGE_FIRE || tree == TALENT_TREE_MAGE_FROST);
    bool priest = playerClass == CLASS_PRIEST &&
        (tree == TALENT_TREE_PRIEST_DISCIPLINE || tree == TALENT_TREE_PRIEST_HOLY);
    if (!warrior && !mage && !priest) return model;
    for (size_t i = 0; i < model.Values.size(); ++i) model.Set(Stat(i), 0);
    for (auto stat : {Stat::Mastery, Stat::Defense, Stat::ArmorPenetration, Stat::BlockValue, Stat::BlockRating})
        model.Mapped[static_cast<size_t>(stat)] = false;
    auto add = [&](Stat stat, float weight) { model.Set(stat, model.Values[static_cast<size_t>(stat)] + weight); };
    add(Stat::Stamina, 0.1f); add(Stat::Armor, 0.001f); add(Stat::Bonus, 1);
    add(Stat::MeleeDps, 0.01f); add(Stat::RangedDps, 0.01f);
    if (warrior && tree != TALENT_TREE_WARRIOR_PROTECTION)
    {
        bool fury = tree == TALENT_TREE_WARRIOR_FURY;
        model.Profile = MeleeDamage;
        add(Stat::Agility, 0.8f); add(Stat::Strength, 2.5f); add(Stat::AttackPower, 0.8f);
        add(Stat::Hit, fury ? 2.3f : 2.0f); add(Stat::Crit, fury ? 2.2f : 1.9f);
        add(Stat::Haste, 0.8f); add(Stat::SpellPower, -2);
        add(Stat::Expertise, fury ? 2.5f : 1.4f); add(Stat::MeleeDps, 7);
    }
    else if (warrior)
    {
        model.Profile = MeleeTank;
        add(Stat::Agility, 0.2f); add(Stat::Strength, 1.3f); add(Stat::Stamina, 3);
        add(Stat::AttackPower, 0.2f); add(Stat::Parry, 2); add(Stat::Dodge, 2);
        add(Stat::Armor, 0.15f); add(Stat::Hit, 2); add(Stat::SpellPower, -2);
        add(Stat::Expertise, 3); add(Stat::MeleeDps, 2);
    }
    else if (mage)
    {
        bool fire = tree == TALENT_TREE_MAGE_FIRE;
        model.Profile = SpellDamage;
        // Native Cata damage gains one spell-power point per intellect above ten.
        add(Stat::Intellect, 0.3f + 1.0f); add(Stat::SpellPower, 1);
        add(Stat::Hit, fire ? 1.2f : 1.1f); add(Stat::Crit, fire ? 1.1f : 0.8f);
        add(Stat::Haste, fire ? 0.8f : 1.0f); add(Stat::AttackPower, -1); add(Stat::RangedDps, 1);
        // No donor Wrath Molten Armor/spirit conversion or obsolete rank check.
    }
    else
    {
        model.Profile = SpellHeal;
        // Native Cata healing has the same intellect-derived spell-power term.
        add(Stat::Intellect, 0.8f + 1.0f); add(Stat::Spirit, 0.6f); add(Stat::HealPower, 1);
        add(Stat::ManaRegen, 0.9f); add(Stat::Crit, 0.6f); add(Stat::Haste, 0.8f);
        add(Stat::AttackPower, -1); add(Stat::RangedDps, 1);
    }
    model.Qualified = true; // Qualified for this bounded heuristic, not optimal gameplay.
    return model;
}
}
#endif
