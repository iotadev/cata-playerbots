/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Base/PlayerbotClassSpellPolicy.h"
#include <catch2/catch.hpp>
#include <limits>

TEST_CASE("Playerbot Spellsteal requires learned spell and a native candidate", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::SpellstealReady(true, true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::SpellstealReady(false, true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::SpellstealReady(true, false, false));
}
TEST_CASE("Playerbot Spellsteal excludes auras marked cannot be stolen", "[playerbot][class-spell]")
{
    REQUIRE_FALSE(PlayerbotClassSpell::SpellstealReady(true, true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::SpellstealReady(false, true, true));
}

TEST_CASE("Playerbot Mage shields retain donor low and medium health boundaries", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::MageDefenseReady(true, false, 44.9f, 45));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, 45, 45));
    REQUIRE(PlayerbotClassSpell::MageDefenseReady(true, false, 64.9f, 65));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, 65, 65));
    REQUIRE(PlayerbotClassSpell::MageDefenseReady(true, false, 100, 65, true));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, true, 100, 65, true));
}

TEST_CASE("Playerbot Ice Block requires critical health learned spell and no blocker", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::MageDefenseReady(true, false, 24.9f, 25));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, 25, 25));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(false, false, 10, 25));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, true, 10, 25));
}

TEST_CASE("Playerbot Mage defenses reject invalid health even under pressure", "[playerbot][class-spell]")
{
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, -1, 65, true));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, 101, 65, true));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, std::numeric_limits<float>::quiet_NaN(), 65, true));
    REQUIRE_FALSE(PlayerbotClassSpell::MageDefenseReady(true, false, std::numeric_limits<float>::infinity(), 65, true));
}

TEST_CASE("Playerbot Brain Freeze requires learned spell and affecting instant cast proc", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::BrainFreezeReady(true, true, -100));
    REQUIRE(PlayerbotClassSpell::BrainFreezeReady(true, true, -100000));
    REQUIRE_FALSE(PlayerbotClassSpell::BrainFreezeReady(true, true, -99));
    REQUIRE_FALSE(PlayerbotClassSpell::BrainFreezeReady(true, false, -100000));
    REQUIRE_FALSE(PlayerbotClassSpell::BrainFreezeReady(false, true, -100000));
}

TEST_CASE("Playerbot Deep Freeze requires learned spell and native frozen eligibility", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::DeepFreezeReady(true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::DeepFreezeReady(true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::DeepFreezeReady(false, true));
}

TEST_CASE("Playerbot Hot Streak requires learned base and current native override", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::HotStreakReady(true, true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::HotStreakReady(false, true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::HotStreakReady(true, false, true));
    REQUIRE_FALSE(PlayerbotClassSpell::HotStreakReady(true, true, false));
}
TEST_CASE("Playerbot Scorch debuff requires affecting native Critical Mass talent", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::CriticalMassScorchReady(true, true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::CriticalMassScorchReady(true, true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::CriticalMassScorchReady(true, false, false));
    REQUIRE_FALSE(PlayerbotClassSpell::CriticalMassScorchReady(false, true, false));
}

TEST_CASE("Playerbot Arcane Missiles requires a learned spell and native proc", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::ArcaneMissilesReady(true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::ArcaneMissilesReady(true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::ArcaneMissilesReady(false, true));
}
TEST_CASE("Playerbot Arcane stack trigger reads the supplied native cap", "[playerbot][class-spell]")
{
    REQUIRE_FALSE(PlayerbotClassSpell::ArcaneBlastAtCap(3, 4));
    REQUIRE(PlayerbotClassSpell::ArcaneBlastAtCap(4, 4));
    REQUIRE_FALSE(PlayerbotClassSpell::ArcaneBlastAtCap(4, 0));
    REQUIRE(PlayerbotClassSpell::ArcaneBlastAtCap(3, 3));
}

TEST_CASE("Playerbot Colossus Smash preserves its current owned debuff window", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::ColossusSmashReady(true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::ColossusSmashReady(true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::ColossusSmashReady(false, false));
    REQUIRE_FALSE(PlayerbotClassSpell::ColossusSmashReady(false, true));
}

TEST_CASE("Playerbot Raging Blow requires learned spell and native Enrage state", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::RagingBlowReady(true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::RagingBlowReady(true, false));
    REQUIRE_FALSE(PlayerbotClassSpell::RagingBlowReady(false, true));
    REQUIRE_FALSE(PlayerbotClassSpell::RagingBlowReady(false, false));
}

TEST_CASE("Playerbot Overpower requires a learned spell and target reaction or affecting proc", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::OverpowerReady(true, true, false));
    REQUIRE(PlayerbotClassSpell::OverpowerReady(true, false, true));
    REQUIRE(PlayerbotClassSpell::OverpowerReady(true, true, true));
    REQUIRE_FALSE(PlayerbotClassSpell::OverpowerReady(true, false, false));
    REQUIRE_FALSE(PlayerbotClassSpell::OverpowerReady(false, true, true));
}

TEST_CASE("Playerbot Fury Slam requires an affecting complete cast time reduction", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::InstantSlamReady(true, true, -100));
    REQUIRE(PlayerbotClassSpell::InstantSlamReady(true, true, -101));
    REQUIRE_FALSE(PlayerbotClassSpell::InstantSlamReady(true, true, -99));
    REQUIRE_FALSE(PlayerbotClassSpell::InstantSlamReady(true, true, 0));
    REQUIRE_FALSE(PlayerbotClassSpell::InstantSlamReady(true, false, -100));
    REQUIRE_FALSE(PlayerbotClassSpell::InstantSlamReady(false, true, -100));
}

TEST_CASE("Playerbot Execute preserves strict donor sub twenty percent boundary", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::ExecuteReady(19.9f));
    REQUIRE_FALSE(PlayerbotClassSpell::ExecuteReady(20.0f));
    REQUIRE_FALSE(PlayerbotClassSpell::ExecuteReady(100.0f));
    REQUIRE_FALSE(PlayerbotClassSpell::ExecuteReady(-1.0f));
    REQUIRE_FALSE(PlayerbotClassSpell::ExecuteReady(std::numeric_limits<float>::quiet_NaN()));
    REQUIRE_FALSE(PlayerbotClassSpell::ExecuteReady(std::numeric_limits<float>::infinity()));
}

TEST_CASE("Playerbot tank defensive health preserves strict donor thresholds", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::DefensiveHealthReady(44.9f, false));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(45.0f, false));
    REQUIRE(PlayerbotClassSpell::DefensiveHealthReady(24.9f, true));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(25.0f, true));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(-1.0f, false));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(100.0f, false));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(std::numeric_limits<float>::quiet_NaN(), false));
    REQUIRE_FALSE(PlayerbotClassSpell::DefensiveHealthReady(std::numeric_limits<float>::infinity(), true));
}

TEST_CASE("Playerbot Sunder refresh uses Cata stack cap and donor duration boundary", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::SunderArmorNeeded(false, 0, 3, 0));
    REQUIRE(PlayerbotClassSpell::SunderArmorNeeded(true, 2, 3, 30000));
    REQUIRE_FALSE(PlayerbotClassSpell::SunderArmorNeeded(true, 3, 3, 6001));
    REQUIRE(PlayerbotClassSpell::SunderArmorNeeded(true, 3, 3, 6000));
    REQUIRE_FALSE(PlayerbotClassSpell::SunderArmorNeeded(true, 3, 3, -1));
    REQUIRE_FALSE(PlayerbotClassSpell::SunderArmorNeeded(false, 0, 0, 0));
    REQUIRE_FALSE(PlayerbotClassSpell::SunderArmorNeeded(true, 5, 5, 30000));
}

TEST_CASE("Playerbot Heroic Strike DPS reserve uses native tenths of rage", "[playerbot][class-spell]")
{
    REQUIRE_FALSE(PlayerbotClassSpell::HeroicStrikeReady(true, 40, false));
    REQUIRE_FALSE(PlayerbotClassSpell::HeroicStrikeReady(true, 399, false));
    REQUIRE(PlayerbotClassSpell::HeroicStrikeReady(true, 400, false));
    REQUIRE_FALSE(PlayerbotClassSpell::HeroicStrikeReady(false, 1000, false));
}
TEST_CASE("Playerbot Heroic Strike tank reserve preserves donor high-rage boundary", "[playerbot][class-spell]")
{
    REQUIRE_FALSE(PlayerbotClassSpell::HeroicStrikeReady(true, 599, true));
    REQUIRE(PlayerbotClassSpell::HeroicStrikeReady(true, 600, true));
    REQUIRE_FALSE(PlayerbotClassSpell::HeroicStrikeReady(false, 1000, true));
}
TEST_CASE("Playerbot Ice Lance requires a known spell and current freeze or proc", "[playerbot][class-spell]")
{
    REQUIRE(PlayerbotClassSpell::IceLanceReady(true, true, false));
    REQUIRE(PlayerbotClassSpell::IceLanceReady(true, false, true));
    REQUIRE_FALSE(PlayerbotClassSpell::IceLanceReady(true, false, false));
    REQUIRE_FALSE(PlayerbotClassSpell::IceLanceReady(false, true, true));
}
