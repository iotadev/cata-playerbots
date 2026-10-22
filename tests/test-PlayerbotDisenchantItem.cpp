/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotDisenchantItem.h"
#include <catch2/catch.hpp>
#include <limits>
TEST_CASE("Disenchant retention uses donor binding and skill with native Cata limits", "[playerbot][inventory][disenchant]")
{
    using PlayerbotDisenchantItem::Eligible;
    REQUIRE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, true, 75, 75, 1));
    REQUIRE(Eligible(ITEM_CLASS_WEAPON, ITEM_QUALITY_RARE, BIND_ON_EQUIP, true, true, 100, 75, 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_WEAPON, ITEM_QUALITY_RARE, BIND_ON_EQUIP, false, true, 100, 75, 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_LEGENDARY, BIND_ON_ACQUIRE, true, true, 100, 75, 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, true, 74, 75, 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, true, 100, uint32(-1), 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, true, 100, 75, 0));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_ARMOR, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, false, 100, 75, 1));
    REQUIRE_FALSE(Eligible(ITEM_CLASS_CONSUMABLE, ITEM_QUALITY_UNCOMMON, BIND_ON_ACQUIRE, true, true, 100, 75, 1));
}
TEST_CASE("Disenchant retention requires qualified non upgrade rows for the exact carried instance", "[playerbot][inventory][disenchant]")
{
    using namespace PlayerbotEquipment;
    Survey survey; survey.Available = true;
    Evaluation row;
    row.Input.Item = ObjectGuid(HighGuid::Item, uint32(10));
    row.Input.Existing = ObjectGuid(HighGuid::Item, uint32(11));
    row.Input.Entry = 100; row.Input.Property = -2; row.Input.SuffixFactor = 50;
    row.LayoutKnown = true; row.CandidateScore = 5; row.ExistingScore = 10; row.Result = Decision::Keep;
    survey.Items.push_back(row);
    auto matches = [&] { return PlayerbotDisenchantItem::NonUpgrade(survey, row.Input.Item, 100, -2, 50); };
    REQUIRE(matches());
    REQUIRE_FALSE(PlayerbotDisenchantItem::NonUpgrade(survey, row.Input.Item, 100, -2, 51));
    REQUIRE_FALSE(PlayerbotDisenchantItem::NonUpgrade(survey, ObjectGuid(HighGuid::Item, uint32(12)), 100, -2, 50));
    survey.Items[0].Result = Decision::Upgrade; REQUIRE_FALSE(matches());
    survey.Items[0].Result = Decision::NeedsRepair; REQUIRE_FALSE(matches());
    survey.Items[0].Result = Decision::Keep; survey.Items[0].ExistingScore.reset(); REQUIRE_FALSE(matches());
    survey.Items[0] = row; survey.Items[0].Input.Existing = row.Input.Item; REQUIRE_FALSE(matches());
    survey.Items[0] = row; survey.Items[0].CandidateScore = std::numeric_limits<float>::quiet_NaN(); REQUIRE_FALSE(matches());
    survey.Items[0] = row; survey.Items[0].ExistingScore = std::numeric_limits<float>::infinity(); REQUIRE_FALSE(matches());
    survey.Items[0] = row; survey.Items[0].Input.Existing.Clear(); REQUIRE_FALSE(matches());
    survey.Items[0] = row; survey.Items.push_back(row); survey.Items[1].Result = Decision::Unknown; REQUIRE_FALSE(matches());
}
