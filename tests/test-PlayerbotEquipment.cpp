/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotEquipment.h"
#include "Player.h"
#include "../src/Ai/Base/PlayerbotStarterGearWeights.h"
#include <catch2/catch.hpp>
using namespace PlayerbotEquipment;
using namespace PlayerbotItemStats;
TEST_CASE("Playerbot starter Warrior weapon penalties retain donor specialization preferences", "[playerbot][inventory]")
{
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS, INVTYPE_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD, true, false) == Approx(0.1f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_SWORD2, true, false) == Approx(0.5f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_SWORD2, true, false) == Approx(0.05f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_SWORD2, true, false) == Approx(0.05f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, INVTYPE_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD, true, false) == Approx(1));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, INVTYPE_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD, false, false) == Approx(0.1f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_SWORD2, true, true) == Approx(0.5f));
    REQUIRE(WeaponMultiplier(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_STAFF, true, true) == Approx(0.05f));
}
TEST_CASE("Playerbot caster weapon penalties retain donor hand weighting", "[playerbot][inventory]")
{
    REQUIRE(WeaponMultiplier(CLASS_MAGE, TALENT_TREE_MAGE_FROST, INVTYPE_WEAPON, ITEM_SUBCLASS_WEAPON_SWORD, false, false) == Approx(0.65f));
    REQUIRE(WeaponMultiplier(CLASS_PRIEST, TALENT_TREE_PRIEST_HOLY, INVTYPE_2HWEAPON, ITEM_SUBCLASS_WEAPON_STAFF, false, false) == Approx(0.5f));
}
TEST_CASE("Playerbot equipment comparisons defer coupled hand layouts", "[playerbot][inventory]")
{
    REQUIRE(LayoutComparable(EQUIPMENT_SLOT_HEAD, false, false, true));
    REQUIRE(LayoutComparable(EQUIPMENT_SLOT_MAINHAND, false, false, true));
    REQUIRE(LayoutComparable(EQUIPMENT_SLOT_MAINHAND, true, true, false));
    REQUIRE_FALSE(LayoutComparable(EQUIPMENT_SLOT_MAINHAND, true, false, false));
    REQUIRE_FALSE(LayoutComparable(EQUIPMENT_SLOT_MAINHAND, false, true, false));
    REQUIRE_FALSE(LayoutComparable(EQUIPMENT_SLOT_MAINHAND, true, true, true));
    REQUIRE_FALSE(LayoutComparable(EQUIPMENT_SLOT_OFFHAND, true, false, true));
}
TEST_CASE("Playerbot equipment evaluation distinguishes fill upgrade repair and same-entry retention", "[playerbot][inventory]")
{
    REQUIRE(Evaluate(20, {}, true, false, false, false, true) == Decision::FillSlot);
    REQUIRE(Evaluate(111, 100, false, false, false, false, true) == Decision::Upgrade);
    REQUIRE(Evaluate(110, 100, false, false, false, false, true) == Decision::Keep);
    REQUIRE(Evaluate(120, 100, false, true, false, false, true) == Decision::Keep);
    REQUIRE(Evaluate(10, 100, false, true, false, true, true) == Decision::ReplaceBroken);
    REQUIRE(Evaluate(120, 100, false, false, true, false, true) == Decision::NeedsRepair);
    REQUIRE(Evaluate(0, {}, true, false, false, false, true) == Decision::Keep);
}
TEST_CASE("Playerbot equipment evaluation keeps incomplete comparisons unknown", "[playerbot][inventory]")
{
    REQUIRE(Evaluate({}, 100, false, false, false, false, true) == Decision::Unknown);
    REQUIRE(Evaluate(120, {}, false, false, false, false, true) == Decision::Unknown);
    REQUIRE(Evaluate(120, 100, false, false, false, false, false) == Decision::Unknown);
    REQUIRE(Evaluate(120, 100, false, false, false, false, true, 0) == Decision::Unknown);
}
TEST_CASE("Playerbot starter gear models map only implemented specs within the level window", "[playerbot][inventory]")
{
    for (uint32 spec : {TALENT_TREE_WARRIOR_ARMS, TALENT_TREE_WARRIOR_FURY, TALENT_TREE_WARRIOR_PROTECTION})
        REQUIRE(StarterModel(CLASS_WARRIOR, spec, 20).Qualified);
    for (uint32 spec : {TALENT_TREE_MAGE_ARCANE, TALENT_TREE_MAGE_FIRE, TALENT_TREE_MAGE_FROST})
        REQUIRE(StarterModel(CLASS_MAGE, spec, 20).Qualified);
    for (uint32 spec : {TALENT_TREE_PRIEST_DISCIPLINE, TALENT_TREE_PRIEST_HOLY})
        REQUIRE(StarterModel(CLASS_PRIEST, spec, 20).Qualified);
    REQUIRE_FALSE(StarterModel(CLASS_PRIEST, TALENT_TREE_PRIEST_SHADOW, 20).Qualified);
    REQUIRE_FALSE(StarterModel(CLASS_WARRIOR, 0, 20).Qualified);
    REQUIRE_FALSE(StarterModel(CLASS_ROGUE, 0, 20).Qualified);
    REQUIRE_FALSE(StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 9).Qualified);
    REQUIRE(StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 10).Qualified);
    REQUIRE(StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 39).Qualified);
    REQUIRE_FALSE(StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 40).Qualified);
}
TEST_CASE("Playerbot Warrior starter rows preserve donor base additions and spec distinctions", "[playerbot][inventory]")
{
    auto arms = StarterModel(CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS, 20);
    auto fury = StarterModel(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, 20);
    auto tank = StarterModel(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION, 20);
    auto weight = [](WeightModel const& row, Stat stat) { return row.Values[static_cast<size_t>(stat)]; };
    REQUIRE(weight(arms, Stat::MeleeDps) == Approx(7.01f));
    REQUIRE(weight(fury, Stat::Hit) == Approx(2.3f));
    REQUIRE(weight(arms, Stat::Hit) == Approx(2));
    REQUIRE(weight(tank, Stat::Stamina) == Approx(3.1f));
    REQUIRE(weight(tank, Stat::Armor) == Approx(0.151f));
    REQUIRE(tank.Profile == MeleeTank);
    REQUIRE(arms.Profile == MeleeDamage);
}
TEST_CASE("Playerbot caster starter rows include Cata intellect power without Wrath Mage spirit", "[playerbot][inventory]")
{
    auto frost = StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 20);
    auto fire = StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FIRE, 20);
    auto heal = StarterModel(CLASS_PRIEST, TALENT_TREE_PRIEST_HOLY, 20);
    auto weight = [](WeightModel const& row, Stat stat) { return row.Values[static_cast<size_t>(stat)]; };
    REQUIRE(weight(frost, Stat::Intellect) == Approx(1.3f));
    REQUIRE(weight(heal, Stat::Intellect) == Approx(1.8f));
    REQUIRE(weight(frost, Stat::Spirit) == 0);
    REQUIRE(weight(fire, Stat::Spirit) == 0);
    REQUIRE(weight(fire, Stat::Crit) > weight(frost, Stat::Crit));
    REQUIRE(heal.Profile == SpellHeal);
}
TEST_CASE("Playerbot starter gear rows leave mastery and legacy tank channels unqualified", "[playerbot][inventory]")
{
    auto model = StarterModel(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION, 20);
    for (auto stat : {Stat::Mastery, Stat::Defense, Stat::ArmorPenetration, Stat::BlockValue, Stat::BlockRating})
        REQUIRE_FALSE(model.Mapped[static_cast<size_t>(stat)]);
}
TEST_CASE("Playerbot plain starter caster comparisons use distinct intellect and power channels", "[playerbot][inventory]")
{
    BaseStats intellect;
    intellect.Available = true; intellect.OwnerClass = CLASS_MAGE; intellect.OwnerSpec = TALENT_TREE_MAGE_FROST;
    intellect.OwnerLevel = 20; intellect.OwnerProfile = SpellDamage;
    auto power = intellect;
    intellect.Add(Stat::Intellect, 10);
    power.AddItemStat(ITEM_MOD_SPELL_POWER, 10, SpellDamage);
    auto model = StarterModel(CLASS_MAGE, TALENT_TREE_MAGE_FROST, 20);
    REQUIRE(*Score(intellect, model) == Approx(13));
    REQUIRE(*Score(power, model) == Approx(10));
    REQUIRE(Compare(Score(intellect, model), Score(power, model), 1.1f) == Comparison::Upgrade);
}
TEST_CASE("Playerbot Cata wands and thrown weapons retain the ranged damage channel", "[playerbot][inventory]")
{
    REQUIRE(WeaponChannel(INVTYPE_RANGED) == Stat::RangedDps);
    REQUIRE(WeaponChannel(INVTYPE_RANGEDRIGHT) == Stat::RangedDps);
    REQUIRE(WeaponChannel(INVTYPE_THROWN) == Stat::RangedDps);
    REQUIRE(WeaponChannel(INVTYPE_WEAPON) == Stat::MeleeDps);
    REQUIRE(WeaponChannel(INVTYPE_2HWEAPON) == Stat::MeleeDps);
}
namespace
{
BaseStats Facts()
{
    BaseStats stats;
    stats.Available = true; stats.OwnerClass = CLASS_MAGE; stats.OwnerSpec = TALENT_TREE_MAGE_FROST;
    stats.OwnerLevel = 20; stats.OwnerProfile = SpellDamage;
    stats.Add(Stat::Intellect, 10);
    return stats;
}
WeightModel Model()
{
    WeightModel weights;
    weights.Qualified = true; weights.Class = CLASS_MAGE; weights.Spec = TALENT_TREE_MAGE_FROST;
    weights.Profile = SpellDamage; weights.Set(Stat::Intellect, 2);
    return weights; // Synthetic test weights, not a shipped Cata table.
}
}
TEST_CASE("Playerbot equipment score requires qualified matching class spec role and level", "[playerbot][inventory]")
{
    auto stats = Facts(); auto model = Model();
    REQUIRE(Score(stats, model) == 20);
    model.Qualified = false; REQUIRE_FALSE(Score(stats, model));
    model = Model(); model.Spec = TALENT_TREE_MAGE_FIRE; REQUIRE_FALSE(Score(stats, model));
    model = Model(); model.Class = CLASS_PRIEST; REQUIRE_FALSE(Score(stats, model));
    model = Model(); model.Profile = SpellHeal; REQUIRE_FALSE(Score(stats, model));
    model = Model(); model.MinimumLevel = 21; REQUIRE_FALSE(Score(stats, model));
    model = Model(); model.MaximumLevel = 19; REQUIRE_FALSE(Score(stats, model));
}
TEST_CASE("Playerbot score distinguishes unmapped from intentionally zero weighted stats", "[playerbot][inventory]")
{
    auto stats = Facts(); auto model = Model();
    stats.Add(Stat::Mastery, 5); REQUIRE_FALSE(Score(stats, model));
    model.Set(Stat::Mastery, 0); REQUIRE(Score(stats, model) == 20);
    model.Set(Stat::Mastery, 3); REQUIRE(Score(stats, model) == 35);
}
TEST_CASE("Playerbot partial equipment inputs stay unavailable for scoring", "[playerbot][inventory]")
{
    for (auto flag : {&BaseStats::UnsupportedStats, &BaseStats::UnsupportedEffects, &BaseStats::HasProcEffects,
        &BaseStats::HasUseEffects, &BaseStats::HasConditionalEffects, &BaseStats::HasSockets, &BaseStats::HasItemSet})
    {
        auto stats = Facts(); stats.*flag = true; REQUIRE_FALSE(Score(stats, Model()));
    }
    auto stats = Facts(); stats.HasRandomProperties = true; REQUIRE_FALSE(Score(stats, Model()));
    stats.AffixResolved = true; stats.AffixPoolUnverified = true; REQUIRE_FALSE(Score(stats, Model()));
    auto verified = stats; verified.AffixInstanceVerified = true;
    REQUIRE(Score(verified, Model()) == 20);
    REQUIRE_FALSE(Score(stats, Model())); // Owned proof cannot leak into cached hypothetical facts.
    auto lootVerified = stats; lootVerified.AffixLootVerified = true;
    REQUIRE(Score(lootVerified, Model()) == 20);
    REQUIRE_FALSE(Score(stats, Model())); // Native loot proof is local too.
    lootVerified.HasProcEffects = true; REQUIRE_FALSE(Score(lootVerified, Model()));
    stats.AffixPoolUnverified = false; REQUIRE(Score(stats, Model()) == 20);
}
TEST_CASE("Playerbot score rejects nonfinite inputs weights and narrowing overflow", "[playerbot][inventory]")
{
    auto stats = Facts(); auto model = Model();
    model.Set(Stat::Intellect, std::numeric_limits<float>::infinity()); REQUIRE_FALSE(Score(stats, model));
    model = Model(); stats.Values[static_cast<size_t>(Stat::Intellect)] = std::numeric_limits<float>::quiet_NaN();
    REQUIRE_FALSE(Score(stats, model));
    stats = Facts(); stats.Values[static_cast<size_t>(Stat::Intellect)] = std::numeric_limits<float>::max();
    REQUIRE_FALSE(Score(stats, Model()));
}
TEST_CASE("Playerbot upgrade comparison preserves strict donor improvement thresholds", "[playerbot][inventory]")
{
    REQUIRE(Compare(111, 100, 1.1f) == Comparison::Upgrade);
    REQUIRE(Compare(110, 100, 1.1f) == Comparison::NotUpgrade);
    REQUIRE(Compare(100, 100, 1) == Comparison::NotUpgrade);
    REQUIRE(Compare(99, 100, 0.5f) == Comparison::NotUpgrade);
    REQUIRE(Compare(1, 0, 1.1f) == Comparison::Upgrade);
    REQUIRE(Compare(0, 0, 1) == Comparison::NotUpgrade);
    REQUIRE(Compare({}, 100, 1) == Comparison::Unknown);
    REQUIRE(Compare(100, {}, 1) == Comparison::Unknown);
    REQUIRE(Compare(100, 50, 0) == Comparison::Unknown);
}
