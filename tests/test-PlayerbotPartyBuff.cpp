/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Ai/Base/PlayerbotPartyBuffStrategy.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Playerbot Mage buff strategy retains donor party trigger and priority", "[PlayerbotPartyBuff]")
{
    PlayerbotPartyBuff::Strategy strategy(nullptr, PlayerbotPartyBuff::MageAction, ACTION_HIGH);
    REQUIRE(strategy.getName() == "buff");
    REQUIRE(strategy.GetType() == STRATEGY_TYPE_NONCOMBAT);
    REQUIRE(strategy.getDefaultActions().empty());
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 1);
    std::unique_ptr<TriggerNode> node(nodes.front());
    REQUIRE(node->getName() == "arcane intellect on party");
    auto handlers = node->getHandlers();
    REQUIRE(handlers.size() == 1);
    REQUIRE(handlers.front().getName() == node->getName());
    REQUIRE(handlers.front().getRelevance() == ACTION_HIGH);
}

TEST_CASE("Playerbot Priest buff priority stays below healing and resurrection", "[PlayerbotPartyBuff]")
{
    PlayerbotPartyBuff::Strategy strategy(nullptr, PlayerbotPartyBuff::PriestAction, ACTION_NORMAL);
    std::vector<TriggerNode*> nodes;
    strategy.InitTriggers(nodes);
    REQUIRE(nodes.size() == 1);
    std::unique_ptr<TriggerNode> node(nodes.front());
    REQUIRE(node->getName() == "power word: fortitude on party");
    auto handlers = node->getHandlers();
    REQUIRE(handlers.front().getName() == node->getName());
    REQUIRE(handlers.front().getRelevance() < ACTION_LIGHT_HEAL + 1);
    REQUIRE(handlers.front().getRelevance() < ACTION_CRITICAL_HEAL + 10);
}

TEST_CASE("Playerbot party buff definitions use Cata single and party aura variants", "[PlayerbotPartyBuff]")
{
    REQUIRE(PlayerbotPartyBuff::Brilliance.SpellId == 1459);
    REQUIRE(PlayerbotPartyBuff::Brilliance.SingleAuraId == 79057);
    REQUIRE(PlayerbotPartyBuff::Brilliance.PartyAuraId == 79058);
    REQUIRE(PlayerbotPartyBuff::Fortitude.SpellId == 21562);
    REQUIRE(PlayerbotPartyBuff::Fortitude.SingleAuraId == 79104);
    REQUIRE(PlayerbotPartyBuff::Fortitude.PartyAuraId == 79105);
}
