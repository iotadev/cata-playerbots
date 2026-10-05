/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotThreatStrategy.h"
#include <catch2/catch.hpp>
#include <limits>
#include <array>

TEST_CASE("Playerbot threat percent keeps donor tank ratio and integer boundaries", "[playerbot][threat]")
{
    REQUIRE(PlayerbotThreat::Percent(79.99f, 100.0f, true, true, false) == 79);
    REQUIRE(PlayerbotThreat::Percent(80.0f, 100.0f, true, true, false) == 80);
    REQUIRE(PlayerbotThreat::Percent(50.0f, 100.0f, true, true, false) == 50);
    REQUIRE(PlayerbotThreat::Percent(100.0f, 100.0f, false, true, false) == 0);
}
TEST_CASE("Playerbot threat percent handles fleeing startup zero and saturation safely", "[playerbot][threat]")
{
    REQUIRE(PlayerbotThreat::Percent(0.0f, 0.0f, true, true, false) == 100);
    REQUIRE(PlayerbotThreat::Percent(0.0f, 0.0f, true, false, false) == 0);
    REQUIRE(PlayerbotThreat::Percent(1.0f, 0.0f, true, true, false) == 255);
    REQUIRE(PlayerbotThreat::Percent(300.0f, 100.0f, true, true, false) == 255);
    REQUIRE(PlayerbotThreat::Percent(100.0f, 0.0f, true, true, true) == 0);
    REQUIRE(PlayerbotThreat::Percent(std::numeric_limits<float>::quiet_NaN(), 100.0f, true, true, false) == 100);
}
TEST_CASE("Playerbot threat multiplier blocks single target damage at donor cutoff only", "[playerbot][threat]")
{
    using Type = Action::ActionThreatType;
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 79) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 80) == 0.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 255) == 0.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, false, 255) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::None, true, 255) == 1.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Aoe, true, 255) == 0.0f);
    REQUIRE(PlayerbotThreat::DamageMultiplier(Type::Single, true, 255, true) == 1.0f);
}
TEST_CASE("Playerbot area threat takes the maximum of freshly resolved attackers", "[playerbot][threat]")
{
    std::array<uint8_t, 4> ratios{0, 49, 80, 50};
    auto identity = [](uint8_t value) { return value; };
    REQUIRE(PlayerbotThreat::MaximumPercent(ratios, identity) == 80);
    REQUIRE(PlayerbotThreat::MaximumPercent(std::array<uint8_t, 0>{}, identity) == 0);
    // The resolver can discard a stale/ineligible GUID; no native pointer is retained.
    REQUIRE(PlayerbotThreat::MaximumPercent(ratios, [](uint8_t value) -> uint8_t
        { return value == 80 ? 0 : value; }) == 50);
}
TEST_CASE("Playerbot area damage passes both donor threat cutoffs without blocking support", "[playerbot][threat]")
{
    using Type = Action::ActionThreatType;
    auto multiplier = [](Type type, uint8_t current, uint8_t area, bool grouped = true, bool neglect = false)
        { return PlayerbotThreat::DamageMultiplier(type, grouped, current, neglect, area); };
    REQUIRE(multiplier(Type::Aoe, 79, 49) == 1.0f);
    REQUIRE(multiplier(Type::Aoe, 0, 50) == 0.0f);
    REQUIRE(multiplier(Type::Aoe, 80, 0) == 0.0f);
    REQUIRE(multiplier(Type::Single, 79, 255) == 1.0f);
    REQUIRE(multiplier(Type::None, 255, 255) == 1.0f);
    REQUIRE(multiplier(Type::Aoe, 255, 255, false) == 1.0f);
    REQUIRE(multiplier(Type::Aoe, 255, 255, true, true) == 1.0f);
}
TEST_CASE("Playerbot neglect threat value preserves donor one shot consumption", "[playerbot][threat]")
{
    PlayerbotThreat::NeglectThreatResetValue value(nullptr);
    REQUIRE_FALSE(value.Get());
    value.Set(true);
    REQUIRE(value.Get());
    REQUIRE_FALSE(value.Get());
    value.Set(true);
    value.Reset();
    REQUIRE_FALSE(value.Get());
}
TEST_CASE("Playerbot focus preserves donor area healing and attacker debuff categories", "[playerbot][focus]")
{
    using namespace PlayerbotThreat;
    using Type = Action::ActionThreatType;
    REQUIRE(FocusMultiplierValue(Type::Aoe, false, false) == 0.0f);
    REQUIRE(FocusMultiplierValue(Type::Aoe, true, false) == 1.0f);
    REQUIRE(FocusMultiplierValue(Type::Single, false, false) == 1.0f);
    REQUIRE(FocusMultiplierValue(Type::None, false, false) == 1.0f);
    REQUIRE(FocusMultiplierValue(Type::Single, false, true) == 0.0f);
    REQUIRE(FocusMultiplierValue(Type::Aoe, true, true) == 0.0f);
    FocusMultiplier multiplier(nullptr);
    REQUIRE(multiplier.GetValue(nullptr) == 1.0f);
}
TEST_CASE("Playerbot hostile self centered spells are area actions rather than friendly buffs", "[playerbot][focus][threat]")
{
    using namespace PlayerbotThreat;
    using Type = Action::ActionThreatType;
    REQUIRE(ClassifySpellTarget(true, false) == Type::Aoe);
    REQUIRE(ClassifySpellTarget(true, true) == Type::None);
    REQUIRE(ClassifySpellTarget(false, false) == Type::Single);
}
