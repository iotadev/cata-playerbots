/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Ai/Base/PlayerbotRestStrategy.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot food strategy keeps donor registry names and low relevance", "[PlayerbotRest]")
{
    PlayerbotRest::FoodStrategy strategy(nullptr);
    REQUIRE(strategy.getName() == "food");
    REQUIRE(strategy.GetType() == STRATEGY_TYPE_NONCOMBAT);
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 2);
    std::unique_ptr<TriggerNode> health(nodes[0]);
    std::unique_ptr<TriggerNode> mana(nodes[1]);
    REQUIRE(health->getName() == "low health");
    REQUIRE(mana->getName() == "low mana");
    REQUIRE(health->getHandlers()[0].getName() == "food");
    REQUIRE(mana->getHandlers()[0].getName() == "drink");
    REQUIRE(health->getHandlers()[0].getRelevance() == 3.0f);
    REQUIRE(mana->getHandlers()[0].getRelevance() < ACTION_LIGHT_HEAL);
}
TEST_CASE("Playerbot food threshold excludes dead and recovered characters", "[PlayerbotRest]")
{
    using namespace PlayerbotRest;
    REQUIRE(Needs(Kind::Food, 39.0f, 100.0f, true));
    REQUIRE_FALSE(Needs(Kind::Food, 40.0f, 0.0f, true));
    REQUIRE_FALSE(Needs(Kind::Food, 0.0f, 0.0f, true));
}
TEST_CASE("Playerbot drink threshold excludes nonmana classes", "[PlayerbotRest]")
{
    using namespace PlayerbotRest;
    REQUIRE(Needs(Kind::Drink, 100.0f, 19.0f, true));
    REQUIRE_FALSE(Needs(Kind::Drink, 100.0f, 20.0f, true));
    REQUIRE_FALSE(Needs(Kind::Drink, 100.0f, 0.0f, false));
}
TEST_CASE("Playerbot rest eligibility rejects disable combat death transfer and mounting", "[PlayerbotRest]")
{
    using PlayerbotRest::CanRest;
    REQUIRE(CanRest(true, true, false, false, false, false));
    REQUIRE_FALSE(CanRest(false, true, false, false, false, false));
    REQUIRE_FALSE(CanRest(true, false, false, false, false, false));
    REQUIRE_FALSE(CanRest(true, true, true, false, false, false));
    REQUIRE_FALSE(CanRest(true, true, false, true, false, false));
    REQUIRE_FALSE(CanRest(true, true, false, false, true, false));
    REQUIRE_FALSE(CanRest(true, true, false, false, false, true));
}
