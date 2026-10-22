/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotProfessionItem.h"
#include <catch2/catch.hpp>
#include <algorithm>
TEST_CASE("Profession material catalog keeps donor gathering associations skill scoped", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    auto mining = MaterialSkills(SKILL_MINING);
    REQUIRE(mining[1] == SKILL_BLACKSMITHING);
    REQUIRE(mining[2] == SKILL_JEWELCRAFTING);
    REQUIRE(mining[3] == SKILL_ENGINEERING);
    REQUIRE(MaterialSkills(SKILL_SKINNING)[1] == SKILL_LEATHERWORKING);
    REQUIRE(MaterialSkills(SKILL_HERBALISM)[1] == SKILL_ALCHEMY);
    REQUIRE(MaterialSkills(SKILL_INSCRIPTION)[0] == SKILL_INSCRIPTION);
    REQUIRE(MaterialSkills(SKILL_ARCHAEOLOGY)[0] == SKILL_ARCHAEOLOGY);
    REQUIRE(MaterialSkills(999)[0] == 0);
}
TEST_CASE("Profession material facts require actual positive reagent or create item identities", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    REQUIRE(MaterialMatch(100, 100, 0, false));
    REQUIRE(MaterialMatch(100, 0, 100, true));
    REQUIRE_FALSE(MaterialMatch(100, -1, 0, false));
    REQUIRE_FALSE(MaterialMatch(0xffffffffu, -1, 0, false));
    REQUIRE_FALSE(MaterialMatch(100, 0, 100, false));
    REQUIRE_FALSE(MaterialMatch(0, 0, 0, true));
    REQUIRE_FALSE(MaterialMatch(100, 101, 102, true));
}
TEST_CASE("Profession material class scope does not reinterpret equipment or supplies", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    REQUIRE(MaterialClass(ITEM_CLASS_TRADE_GOODS));
    REQUIRE(MaterialClass(ITEM_CLASS_MISCELLANEOUS));
    REQUIRE(MaterialClass(ITEM_CLASS_REAGENT));
    REQUIRE(MaterialClass(ITEM_CLASS_GEM));
    REQUIRE_FALSE(MaterialClass(ITEM_CLASS_WEAPON));
    REQUIRE_FALSE(MaterialClass(ITEM_CLASS_CONSUMABLE));
    REQUIRE_FALSE(MaterialClass(ITEM_CLASS_RECIPE));
}
TEST_CASE("Profession tools retain the donor's multi skill associations", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    REQUIRE(ToolSkills(2901)[0] == SKILL_MINING);
    auto knife = ToolSkills(40772);
    REQUIRE(std::find(knife.begin(), knife.end(), uint32(SKILL_ENGINEERING)) != knife.end());
    REQUIRE(std::find(knife.begin(), knife.end(), uint32(SKILL_COOKING)) != knife.end());
    REQUIRE(ToolSkills(5956)[1] == SKILL_ENGINEERING);
    REQUIRE(ToolSkills(6218)[0] == SKILL_ENCHANTING);
    REQUIRE(ToolSkills(7005)[0] == SKILL_SKINNING);
    REQUIRE(ToolSkills(6256)[0] == SKILL_FISHING);
    REQUIRE(ToolSkills(159) == std::array<uint32, 5>{});
}
TEST_CASE("Recipe retention maps native Cata professions without claiming books", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    REQUIRE(RecipeSkill(ITEM_SUBCLASS_COOKING_RECIPE) == SKILL_COOKING);
    REQUIRE(RecipeSkill(ITEM_SUBCLASS_JEWELCRAFTING_RECIPE) == SKILL_JEWELCRAFTING);
    REQUIRE(RecipeSkill(ITEM_SUBCLASS_INSCRIPTION_TECHNIQUE) == SKILL_INSCRIPTION);
    REQUIRE(RecipeSkill(ITEM_SUBCLASS_BOOK) == 0);
    REQUIRE(RecipeSkill(999) == 0);
}
TEST_CASE("Recipe retention needs qualified unlearned spells and the owner's skill", "[playerbot][inventory][profession]")
{
    using PlayerbotProfessionItem::RetainRecipe;
    REQUIRE(RetainRecipe(true, true, true));
    REQUIRE_FALSE(RetainRecipe(false, true, true));
    REQUIRE_FALSE(RetainRecipe(true, false, true));
    REQUIRE_FALSE(RetainRecipe(true, true, false));
}
TEST_CASE("Recipe retention follows native Cata generic learn wrapper targets", "[playerbot][inventory][profession]")
{
    using namespace PlayerbotProfessionItem;
    REQUIRE(GenericRecipeTarget(483, 2, 100) == 100);
    REQUIRE(GenericRecipeTarget(55884, 2, 100) == 100);
    REQUIRE(GenericRecipeTarget(483, 1, 100) == 0);
    REQUIRE(GenericRecipeTarget(483, 2, 0) == 0);
    REQUIRE(GenericRecipeTarget(999, 2, 100) == 0);
}
