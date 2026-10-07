/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotConsumableUsage.h"
#include <catch2/catch.hpp>
#include <limits>
using namespace PlayerbotConsumable;
TEST_CASE("Playerbot consumable qualifiers reject malformed and overflow identities", "[playerbot][inventory]")
{
    REQUIRE(ParseItem("3770") == 3770);
    REQUIRE(ParseItem("4294967295") == std::numeric_limits<uint32>::max());
    for (auto text : {"", "0", "-1", "1,2", " 3770", "3770x", "4294967296", "12345678901"})
        REQUIRE(ParseItem(text) == 0);
}
TEST_CASE("Playerbot consumable category translation respects mana and class", "[playerbot][inventory]")
{
    REQUIRE(BaseType(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_FOOD, SPELL_CATEGORY_FOOD, false) == Type::Food);
    REQUIRE(BaseType(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_FOOD, SPELL_CATEGORY_DRINK, true) == Type::Drink);
    REQUIRE(BaseType(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_FOOD, SPELL_CATEGORY_DRINK, false) == Type::None);
    REQUIRE(BaseType(ITEM_CLASS_CONSUMABLE, ITEM_SUBCLASS_BANDAGE, 0, false) == Type::Bandage);
    REQUIRE(BaseType(ITEM_CLASS_WEAPON, ITEM_SUBCLASS_BANDAGE, 0, true) == Type::None);
}
TEST_CASE("Playerbot consumable stock preserves donor two and three stack boundaries", "[playerbot][inventory]")
{
    REQUIRE(StockUsage(true, true, false, 0, 0) == Usage::Use);
    REQUIRE(StockUsage(true, true, false, 1.99f, 0) == Usage::Use);
    REQUIRE(StockUsage(true, true, false, 2, 0) == Usage::Keep);
    REQUIRE(StockUsage(true, true, false, 2.99f, 0) == Usage::Keep);
    REQUIRE(StockUsage(true, true, false, 3, 0) == Usage::None);
    REQUIRE(StockUsage(true, true, false, 0.5f, 1.5f) == Usage::Keep);
    REQUIRE(StockUsage(true, true, false, 0, 2) == Usage::None);
}
TEST_CASE("Playerbot consumable stock distinguishes unsupported and unavailable supply", "[playerbot][inventory]")
{
    REQUIRE(StockUsage(false, true, false, 0, 0) == Usage::Unsupported);
    REQUIRE(StockUsage(true, false, false, 0, 0) == Usage::None);
    REQUIRE(StockUsage(true, true, true, 0, 0) == Usage::None);
    REQUIRE(StockUsage(true, true, false, -1, 0) == Usage::None);
    REQUIRE(StockUsage(true, true, false, 0, std::numeric_limits<float>::infinity()) == Usage::None);
}
