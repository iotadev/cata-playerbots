/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotVendorSale.h"
#include <catch2/catch.hpp>
TEST_CASE("Vendor sale needs every complete retention and owned inventory fact", "[playerbot][inventory][vendor]")
{
    using namespace PlayerbotVendorSale;
    Facts facts{true, true, true, true, true, false, false, 2, 10};
    REQUIRE(Allowed(facts));
    auto original = facts;
    facts.NoProfessionNeeds = false; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.NoGuildNeeds = false; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.QuestFactsKnown = false; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.QuestNeeded = true; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.Busy = true; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.OwnedCarried = false; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.Count = 0; REQUIRE_FALSE(Allowed(facts));
    facts = original; facts.SellPrice = 0; REQUIRE_FALSE(Allowed(facts));
}
TEST_CASE("Vendor sale is a plain junk subset not a general poor quality sweep", "[playerbot][inventory][vendor]")
{
    using namespace PlayerbotVendorSale;
    auto check = [&](uint32 cls, uint32 sub, uint32 quality, uint32 binding, uint32 quest, uint32 skill, uint32 totem, bool effects, bool affix)
    { return PlainTemplate(cls, sub, quality, binding, quest, skill, totem, effects, affix); };
    REQUIRE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_WEAPON, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK_REAGENT, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_UNCOMMON, BIND_NONE, 0, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_ON_ACQUIRE, 0, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 1, 0, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 1, 0, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 1, false, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 0, true, false));
    REQUIRE_FALSE(check(ITEM_CLASS_MISCELLANEOUS, ITEM_SUBCLASS_JUNK, ITEM_QUALITY_POOR, BIND_NONE, 0, 0, 0, false, true));
}
TEST_CASE("Vendor sale has an exact named command without all item aliases", "[playerbot][inventory][vendor]")
{
    REQUIRE(PlayerbotVendorSale::Command("sell vendor"));
    for (auto text : {"sell", "sell *", "sell gray", "sell vendor all", "sell 100"})
        REQUIRE_FALSE(PlayerbotVendorSale::Command(text));
}
TEST_CASE("Vendor sale protects any active quest reference independent of carried completion counts", "[playerbot][inventory][vendor]")
{
    REQUIRE(PlayerbotVendorSale::QuestReference(100, 100, 0));
    REQUIRE(PlayerbotVendorSale::QuestReference(100, 0, 100));
    REQUIRE_FALSE(PlayerbotVendorSale::QuestReference(0, 0, 0));
    REQUIRE_FALSE(PlayerbotVendorSale::QuestReference(100, 101, 102));
}
