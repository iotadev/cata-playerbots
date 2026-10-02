/* Released under GNU GPL v2 or any later version. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotTargetSelection.h"
#include <catch2/catch.hpp>
#include <limits>
using PlayerbotTargetSelection::Candidate;

TEST_CASE("Playerbot fallback routes unassigned DPS Warriors without pretending they are tanks", "[playerbot][target]")
{
    using PlayerbotTargetSelection::FallbackValue;
    REQUIRE(std::string(FallbackValue(CLASS_WARRIOR, 0, true, false, false)) == "dps target");
    REQUIRE(std::string(FallbackValue(CLASS_WARRIOR, TALENT_TREE_WARRIOR_ARMS, true, false, false)) == "dps target");
    REQUIRE(std::string(FallbackValue(CLASS_WARRIOR, TALENT_TREE_WARRIOR_FURY, true, false, false)) == "dps target");
    REQUIRE(std::string(FallbackValue(CLASS_WARRIOR, TALENT_TREE_WARRIOR_PROTECTION, true, false, false)) == "tank target");
    REQUIRE(std::string(FallbackValue(CLASS_MAGE, 0, false, true, false)) == "dps target");
    REQUIRE(std::string(FallbackValue(CLASS_PRIEST, 0, false, false, true)) == "dps target");
    REQUIRE(FallbackValue(CLASS_WARRIOR, 0, false, true, true) == nullptr);
    REQUIRE(FallbackValue(CLASS_MAGE, 0, true, false, true) == nullptr);
    REQUIRE(FallbackValue(CLASS_PRIEST, 0, true, true, false) == nullptr);
    REQUIRE(FallbackValue(CLASS_ROGUE, 0, true, true, true) == nullptr);
}

TEST_CASE("Playerbot tank target bands prioritize lost aggro before melee and distant owned targets", "[playerbot][target]")
{
    using PlayerbotTargetSelection::TankBand;
    using PlayerbotTargetSelection::BetterTank;
    REQUIRE(TankBand({10, 5, false, false}) == 2);
    REQUIRE(TankBand({2, 5, true, true}) == 1);
    REQUIRE(TankBand({10, 5, true, false}) == 0);
    REQUIRE(BetterTank({10, 100, false, false}, {2, 5, true, true}));
    REQUIRE(BetterTank({2, 100, true, true}, {10, 5, true, false}));
}
TEST_CASE("Playerbot tank target ties use distance for lost aggro and lower own threat otherwise", "[playerbot][target]")
{
    using PlayerbotTargetSelection::BetterTank;
    REQUIRE(BetterTank({5, 100, false, false}, {10, 0, false, false}));
    REQUIRE_FALSE(BetterTank({10, 0, false, false}, {5, 100, false, false}));
    REQUIRE(BetterTank({2, 5, true, true}, {1, 10, true, true}));
    REQUIRE(BetterTank({15, 5, true, false}, {10, 10, true, false}));
    REQUIRE_FALSE(BetterTank({2, 5, true, true}, {2, 5, true, true}));
}
TEST_CASE("Playerbot tank aggro subset accepts self and another recognized tank victim", "[playerbot][target]")
{
    using PlayerbotTargetSelection::HasTankAggro;
    REQUIRE(HasTankAggro(false, false, false));
    REQUIRE(HasTankAggro(true, true, false));
    REQUIRE(HasTankAggro(true, false, true));
    REQUIRE_FALSE(HasTankAggro(true, false, false));
}

TEST_CASE("Playerbot target icons retain donor indices and reject unknown names", "[playerbot][target]")
{
    using PlayerbotTargetSelection::IconIndex;
    REQUIRE(IconIndex("star") == 0);
    REQUIRE(IconIndex("moon") == 4);
    REQUIRE(IconIndex("skull") == 7);
    REQUIRE(IconIndex("cross") == 6);
    REQUIRE(IconIndex("") == -1);
    REQUIRE(IconIndex("Skull") == -1);
}
TEST_CASE("Playerbot caster target ranking preserves donor lifetime bands and boundaries", "[playerbot][target]")
{
    using PlayerbotTargetSelection::LifetimeBand;
    using PlayerbotTargetSelection::Better;
    REQUIRE(LifetimeBand({10, 4.99f, true, false, false}) == 11);
    REQUIRE(LifetimeBand({10, 5, true, false, false}) == 12);
    REQUIRE(LifetimeBand({10, 30, true, false, false}) == 12);
    REQUIRE(LifetimeBand({10, 30.01f, true, false, false}) == 10);
    REQUIRE(Better({10, 10, true, false, false}, {10, 3, true, false, false}, true));
    REQUIRE(Better({10, 3, true, false, false}, {10, 40, true, false, false}, true));
    REQUIRE(Better({10, 6, true, false, false}, {10, 20, true, false, false}, true));
    REQUIRE(Better({10, 40, true, false, false}, {10, 60, true, false, false}, true));
    REQUIRE(Better({10, 2, true, true, false}, {10, 4, true, false, false}, true));
    REQUIRE_FALSE(Better({10, 4, true, false, false}, {10, 2, true, true, false}, true));
    REQUIRE(Better({10, 4, true, false, false}, {10, 2, true, false, false}, true));
}
TEST_CASE("Playerbot general target ranking prioritizes range then lifetime or distance", "[playerbot][target]")
{
    using PlayerbotTargetSelection::Better;
    REQUIRE(Better({10, 60, true, false, false}, {40, 5, false, false, false}, false));
    REQUIRE(Better({20, 5, true, false, false}, {10, 10, true, false, false}, false));
    REQUIRE(Better({35, 50, false, false, false}, {45, 3, false, false, false}, false));
    REQUIRE_FALSE(Better({10, 20, true, false, false}, {10, 20, true, false, false}, false));
}
TEST_CASE("Playerbot target priorities precede ranking and unavailable DPS is explicit", "[playerbot][target]")
{
    using PlayerbotTargetSelection::Better;
    using PlayerbotTargetSelection::ValidEstimate;
    REQUIRE(Better({20, 60, true, false, true}, {10, 10, true, false, false}, true));
    REQUIRE_FALSE(Better({10, 10, true, false, true}, {20, 60, true, false, true}, true));
    REQUIRE_FALSE(Better({10, 10, true, false, false}, {20, 60, true, false, true}, false));
    REQUIRE(ValidEstimate(1));
    REQUIRE_FALSE(ValidEstimate(0));
    REQUIRE_FALSE(ValidEstimate(-1));
    REQUIRE_FALSE(ValidEstimate(std::numeric_limits<float>::infinity()));
    REQUIRE_FALSE(ValidEstimate(std::numeric_limits<float>::quiet_NaN()));
}
