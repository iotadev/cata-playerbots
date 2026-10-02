/* Released under GNU GPL v2 or any later version. See AUTHORS.md. */
#include "../src/Ai/Class/Mage/PlayerbotMageArmorStrategy.h"
#include <catch2/catch.hpp>
#include <memory>

TEST_CASE("Mage armor policy prefers active specialization and only learned spells", "[PlayerbotMageArmor]")
{
    using namespace PlayerbotMageArmor;
    REQUIRE(Select(true, true, true, true) == Mage);
    REQUIRE(Select(false, true, true, true) == Molten);
    REQUIRE(Select(true, false, true, true) == Molten);
    REQUIRE(Select(false, true, false, true) == Mage);
    REQUIRE(Select(false, false, false, true) == Frost);
    REQUIRE(Select(true, false, false, false) == 0);
    REQUIRE(Frost == 7302);
}
TEST_CASE("Mage armor strategies retain donor names and priorities", "[PlayerbotMageArmor]")
{
    using namespace PlayerbotMageArmor;
    for (bool mana : { true, false })
    {
        ArmorStrategy strategy(nullptr, mana);
        REQUIRE(strategy.getName() == (mana ? "bmana" : "bdps"));
        REQUIRE(strategy.GetType() == STRATEGY_TYPE_NONCOMBAT);
        std::vector<TriggerNode*> nodes;
        strategy.InitTriggers(nodes);
        REQUIRE(nodes.size() == (mana ? 2 : 1));
        for (TriggerNode* node : nodes)
        {
            std::unique_ptr<TriggerNode> owned(node);
            REQUIRE(node->getName() == node->getHandlers()[0].getName());
            REQUIRE(node->getHandlers()[0].getRelevance() == 19.0f);
        }
    }
}
