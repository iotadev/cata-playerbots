/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Ai/Base/PlayerbotRestStrategy.h"
#include "../src/Ai/Base/PlayerbotRestItem.h"
#include "../src/Ai/Base/PlayerbotTargetSelection.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot recovery item metadata matches donor food subclasses and category precedence", "[PlayerbotRest]")
{
    using namespace PlayerbotRest;
    REQUIRE(FoodItem(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_FOOD));
    REQUIRE(FoodItem(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_CONSUMABLE));
    REQUIRE_FALSE(FoodItem(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_POTION));
    REQUIRE_FALSE(FoodItem(ITEM_CLASS_WEAPON, ITEM_SUBCLASS_FOOD));
    REQUIRE(ItemCategory(SPELL_CATEGORY_DRINK, SPELL_CATEGORY_FOOD) == SPELL_CATEGORY_DRINK);
    REQUIRE(ItemCategory(0, SPELL_CATEGORY_FOOD) == SPELL_CATEGORY_FOOD);
    REQUIRE(ItemCategory(7, SPELL_CATEGORY_DRINK) == 7);
    REQUIRE(ItemCategory(0, 0) == 0);
}
TEST_CASE("Playerbot rest completion preserves the selected recovery mode and clears it", "[PlayerbotRest]")
{
    PlayerbotRest::ActiveRest rest;
    rest.Begin(123, true);
    REQUIRE(rest.Spell == 123);
    REQUIRE_FALSE(rest.Finished(100, 94));
    REQUIRE(rest.Finished(10, 95));
    rest.Begin(456, false);
    REQUIRE(rest.Spell == 456);
    REQUIRE_FALSE(rest.Finished(94, 100));
    REQUIRE(rest.Finished(95, 10));
    rest.Clear();
    REQUIRE(rest.Spell == 0);
    REQUIRE_FALSE(rest.Drinking);
}

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

TEST_CASE("Playerbot rest waits for eligible attached party combat without blocking unrelated members", "[PlayerbotRest]")
{
    using PlayerbotTargetSelection::CombatScopeAllows;
    auto canRest = [](bool partyCombat)
    {
        return PlayerbotRest::CanRest(true, true, false, false, false, partyCombat);
    };
    REQUIRE_FALSE(canRest(CombatScopeAllows(true, false, false, false)));
    REQUIRE_FALSE(canRest(CombatScopeAllows(false, true, true, true)));
    REQUIRE(canRest(CombatScopeAllows(false, false, true, true)));
    REQUIRE(canRest(CombatScopeAllows(false, true, false, true)));
    REQUIRE(canRest(CombatScopeAllows(false, true, true, false)));
}
