/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotItemStats.h"
#include <catch2/catch.hpp>
#include <limits>
using namespace PlayerbotItemStats;
TEST_CASE("Playerbot item stat queries bound item and signed affix identities", "[playerbot][inventory]")
{
    REQUIRE(ParseQuery("3770").Item == 3770);
    REQUIRE(ParseQuery("100,-25").Property == -25);
    REQUIRE(ParseQuery("100,25").Property == 25);
    REQUIRE(ParseQuery("100,0").Item == 100);
    for (auto text : {"", "0", "1,", "1,2,3", "1,+2", "1, 2", "4294967296", "1,2147483648", "1,-2147483649"})
        REQUIRE_FALSE(bool(ParseQuery(text)));
}
TEST_CASE("Playerbot signed affix magnitude avoids minimum integer overflow", "[playerbot][inventory]")
{
    REQUIRE(PropertyIdentity(25) == 25);
    REQUIRE(PropertyIdentity(-25) == 25);
    REQUIRE(PropertyIdentity(std::numeric_limits<int32>::min()) == uint32(2147483648u));
}
TEST_CASE("Playerbot suffix allocations use wide multiplication and native truncation", "[playerbot][inventory]")
{
    REQUIRE(SuffixAmount(3333, 100) == 33);
    REQUIRE(SuffixAmount(0, 100) == 0);
    REQUIRE(SuffixAmount(10000, 500000) == 500000);
    REQUIRE_FALSE(SuffixAmount(std::numeric_limits<uint32>::max(), std::numeric_limits<uint32>::max()));
}
TEST_CASE("Playerbot socket heuristic preserves donor three percent per native socket", "[playerbot][inventory]")
{
    REQUIRE(*SocketMultiplier(0) == Approx(1));
    REQUIRE(*SocketMultiplier(1) == Approx(1.03f));
    REQUIRE(*SocketMultiplier(MAX_ITEM_PROTO_SOCKETS) == Approx(1.09f));
    REQUIRE_FALSE(SocketMultiplier(MAX_ITEM_PROTO_SOCKETS + 1));
}
TEST_CASE("Playerbot set heuristic distinguishes first piece partial set and completed thresholds", "[playerbot][inventory]")
{
    REQUIRE(SetMultiplier(false, false, 0, 0) == Approx(1));
    REQUIRE(SetMultiplier(true, false, 0, 4) == Approx(1.05f));
    REQUIRE(SetMultiplier(true, true, 0, 4) == Approx(1));
    REQUIRE(SetMultiplier(true, true, 2, 4) == Approx(1.2f));
    REQUIRE(SetMultiplier(true, true, 4, 4) == Approx(1));
    REQUIRE(SetMultiplier(true, true, 5, 4) == Approx(1));
    REQUIRE(SetMultiplier(true, true, 0, 0) == Approx(1));
}
TEST_CASE("Playerbot flat aura collection handles individual and all primary stats", "[playerbot][inventory]")
{
    BaseStats stats;
    REQUIRE(stats.AddFlatAura(SPELL_AURA_MOD_STAT, -1, 3, SpellHeal));
    REQUIRE(stats.AddFlatAura(SPELL_AURA_MOD_STAT, int32(StatType::Intellect), 7, SpellHeal));
    REQUIRE(stats.Get(Stat::Intellect) == 10);
    for (auto stat : {Stat::Strength, Stat::Agility, Stat::Stamina, Stat::Spirit}) REQUIRE(stats.Get(stat) == 3);
    REQUIRE(stats.AddFlatAura(SPELL_AURA_MOD_STAT, int32(StatType::AllPrimaryStats2), 2, SpellHeal));
    REQUIRE(stats.Get(Stat::Intellect) == 12);
    REQUIRE(stats.Get(Stat::Spirit) == 5);
    REQUIRE_FALSE(stats.AddFlatAura(SPELL_AURA_MOD_STAT, 100, 10, SpellHeal));
    REQUIRE(stats.UnsupportedEffects);
}
TEST_CASE("Playerbot aura rating masks preserve type filtering mastery and unknown bits", "[playerbot][inventory]")
{
    BaseStats stats;
    uint32 mask = (uint32(1) << CR_HIT_MELEE) | (uint32(1) << CR_HIT_SPELL) | (uint32(1) << CR_MASTERY);
    REQUIRE(stats.AddRating(mask, 12, SpellHeal));
    REQUIRE(stats.Get(Stat::Hit) == 12);
    REQUIRE(stats.Get(Stat::Mastery) == 12);
    REQUIRE_FALSE(stats.AddRating(uint32(1) << 31, 2, SpellHeal));
    REQUIRE_FALSE(stats.AddRating(0, std::numeric_limits<float>::infinity(), SpellHeal));
    REQUIRE(stats.UnsupportedEffects);
}
TEST_CASE("Playerbot flat equipment auras retain donor role and school semantics", "[playerbot][inventory]")
{
    BaseStats healer, melee, ranged;
    REQUIRE(healer.AddFlatAura(SPELL_AURA_MOD_DAMAGE_DONE, SPELL_SCHOOL_MASK_MAGIC, 20, SpellHeal));
    REQUIRE(healer.Get(Stat::SpellPower) == 20);
    REQUIRE(healer.Get(Stat::HealPower) == 0); // Damage-done aura is not item spell power.
    REQUIRE(healer.AddFlatAura(SPELL_AURA_MOD_HEALING_DONE, 0, 15, SpellHeal));
    REQUIRE(healer.AddFlatAura(SPELL_AURA_MOD_POWER_REGEN, POWER_MANA, 5, SpellHeal));
    REQUIRE(healer.Get(Stat::HealPower) == 15);
    REQUIRE(healer.Get(Stat::ManaRegen) == 5);
    REQUIRE(melee.AddFlatAura(SPELL_AURA_MOD_ATTACK_POWER, 0, 10, MeleeTank));
    REQUIRE(ranged.AddFlatAura(SPELL_AURA_MOD_ATTACK_POWER, 0, 10, Ranged));
    REQUIRE(ranged.AddFlatAura(SPELL_AURA_MOD_RANGED_ATTACK_POWER, 0, 30, Ranged));
    REQUIRE(melee.Get(Stat::AttackPower) == 10);
    REQUIRE(ranged.Get(Stat::AttackPower) == 30);
}
TEST_CASE("Playerbot equipment spell average is deterministic for dice and scaling variance", "[playerbot][inventory]")
{
    REQUIRE(AveragePoints(10, 0) == 10);
    REQUIRE(AveragePoints(10, 1) == 11);
    REQUIRE(AveragePoints(10, 5) == 13);
    REQUIRE(AveragePoints(10, -5) == 8);
    REQUIRE(AveragePoints(10, 5, true) == 10);
}
TEST_CASE("Playerbot partial item effects cannot silently become supported zero stats", "[playerbot][inventory]")
{
    BaseStats stats;
    REQUIRE_FALSE(stats.AddFlatAura(SPELL_AURA_MOD_DAMAGE_DONE, SPELL_SCHOOL_MASK_FIRE, 10, SpellDamage));
    REQUIRE_FALSE(stats.AddFlatAura(SPELL_AURA_MOD_POWER_REGEN, POWER_RAGE, 2, MeleeDamage));
    REQUIRE_FALSE(stats.AddFlatAura(SPELL_AURA_MOD_STAT, -1, std::numeric_limits<float>::quiet_NaN(), SpellHeal));
    REQUIRE_FALSE(stats.AddFlatAura(SPELL_AURA_PROC_TRIGGER_SPELL, 0, 10, MeleeDamage));
    REQUIRE(stats.UnsupportedEffects);
    REQUIRE(stats.Get(Stat::ManaRegen) == 0);
    REQUIRE(stats.Get(Stat::Strength) == 0);
}
TEST_CASE("Playerbot item collector retains donor profile precedence", "[playerbot][inventory]")
{
    REQUIRE(ChooseProfile(true, true, true, true) == SpellHeal);
    REQUIRE(ChooseProfile(false, true, true, true) == SpellDamage);
    REQUIRE(ChooseProfile(false, false, true, true) == MeleeTank);
    REQUIRE(ChooseProfile(false, false, false, true) == MeleeDamage);
    REQUIRE(ChooseProfile(false, false, false, false) == Ranged);
}
TEST_CASE("Playerbot base stats preserve donor resource conversion and additive spell power", "[playerbot][inventory]")
{
    BaseStats stats;
    stats.AddItemStat(ITEM_MOD_MANA, 100, SpellHeal);
    stats.AddItemStat(ITEM_MOD_HEALTH, 150, SpellHeal);
    stats.AddItemStat(ITEM_MOD_STAMINA, 5, SpellHeal);
    stats.AddItemStat(ITEM_MOD_SPELL_POWER, 20, SpellHeal);
    stats.AddItemStat(ITEM_MOD_INTELLECT, -2, SpellHeal);
    REQUIRE(stats.Get(Stat::ManaRegen) == 10);
    REQUIRE(stats.Get(Stat::Stamina) == 15);
    REQUIRE(stats.Get(Stat::SpellPower) == 20);
    REQUIRE(stats.Get(Stat::HealPower) == 20);
    REQUIRE(stats.Get(Stat::Intellect) == -2);
}
TEST_CASE("Playerbot typed ratings and ranged attack power follow collector profile", "[playerbot][inventory]")
{
    for (auto profile : {MeleeDamage, MeleeTank, Ranged, SpellDamage, SpellHeal})
    {
        BaseStats stats;
        stats.AddItemStat(ITEM_MOD_HIT_MELEE_RATING, 1, profile);
        stats.AddItemStat(ITEM_MOD_HIT_RANGED_RATING, 2, profile);
        stats.AddItemStat(ITEM_MOD_HIT_SPELL_RATING, 3, profile);
        stats.AddItemStat(ITEM_MOD_HIT_RATING, 4, profile);
        stats.AddItemStat(ITEM_MOD_RANGED_ATTACK_POWER, 10, profile);
        REQUIRE(stats.Get(Stat::Hit) == (profile == Ranged ? 6 : (profile == SpellDamage || profile == SpellHeal ? 7 : 5)));
        REQUIRE(stats.Get(Stat::AttackPower) == (profile == Ranged ? 10 : 0));
    }
}
TEST_CASE("Playerbot Cata mastery stays distinct and incomplete facts remain explicit", "[playerbot][inventory]")
{
    BaseStats stats;
    REQUIRE_FALSE(stats.Available);
    REQUIRE(stats.AddItemStat(ITEM_MOD_MASTERY_RATING, 9, MeleeTank));
    REQUIRE(stats.AddItemStat(ITEM_MOD_EXTRA_ARMOR, 30, MeleeTank));
    REQUIRE(stats.Get(Stat::Mastery) == 9);
    REQUIRE(stats.Get(Stat::Defense) == 0);
    REQUIRE(stats.Get(Stat::ArmorPenetration) == 0);
    REQUIRE(stats.Get(Stat::Armor) == 30);
    REQUIRE(stats.AddItemStat(-1, 5, MeleeTank));
    REQUIRE_FALSE(stats.AddItemStat(1000, 5, MeleeTank));
    REQUIRE_FALSE(stats.AddItemStat(ITEM_MOD_STAMINA, std::numeric_limits<float>::infinity(), MeleeTank));
    REQUIRE(stats.UnsupportedStats);
    REQUIRE(stats.Get(Stat::Stamina) == 0);
}
