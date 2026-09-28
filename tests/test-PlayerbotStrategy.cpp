/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Engine/Strategy/Strategy.h"
#include <catch2/catch.hpp>
#include <memory>

namespace
{
class TestStrategy final : public Strategy
{
public:
    TestStrategy() : Strategy(nullptr) { }
    std::string const getName() override { return "test strategy"; }
    uint32_t GetType() const override { return STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED; }
};
}

TEST_CASE("Playerbot strategy retains donor fallback action-node links", "[playerbot][engine][strategy]")
{
    TestStrategy strategy;
    std::unique_ptr<ActionNode> healthstone(strategy.GetAction("healthstone"));
    REQUIRE(healthstone != nullptr);
    auto alternatives = healthstone->getAlternatives();
    REQUIRE(alternatives.size() == 1);
    REQUIRE(alternatives[0].getName() == "healing potion");

    std::unique_ptr<ActionNode> nearNode(strategy.GetAction("be near"));
    REQUIRE(nearNode != nullptr);
    REQUIRE(nearNode->getAlternatives()[0].getName() == "follow");
    REQUIRE(strategy.GetAction("unregistered action") == nullptr);
    REQUIRE(strategy.GetType() == (STRATEGY_TYPE_COMBAT | STRATEGY_TYPE_RANGED));
}
