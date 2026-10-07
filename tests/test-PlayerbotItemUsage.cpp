/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotItemUsage.h"
#include <catch2/catch.hpp>
using namespace PlayerbotItemUsage;
TEST_CASE("Playerbot unowned template comparison refuses unproven random affixes", "[playerbot][inventory]")
{
    REQUIRE(PlayerbotEquipment::TemplateComparable(false, false, 0));
    REQUIRE_FALSE(PlayerbotEquipment::TemplateComparable(true, false, 0));
    REQUIRE_FALSE(PlayerbotEquipment::TemplateComparable(false, true, 0));
    REQUIRE_FALSE(PlayerbotEquipment::TemplateComparable(false, false, 1));
    REQUIRE_FALSE(PlayerbotEquipment::TemplateComparable(false, false, -1));
}
TEST_CASE("Playerbot template usage cannot imply an owned item identity", "[playerbot][inventory]")
{
    PlayerbotEquipment::Survey survey;
    survey.Available = true;
    PlayerbotEquipment::Evaluation row;
    row.Input.Entry = 9758;
    row.Result = PlayerbotEquipment::Decision::FillSlot;
    survey.Items.push_back(row);
    Fact fact{SurveyUsage(survey, 9758, 0), Scope::TemplateEquipment};
    REQUIRE(fact.Result == Usage::Equip);
    REQUIRE(fact.Source != Scope::CarriedEquipment);
    REQUIRE(survey.Items.front().Input.Item.IsEmpty());
    REQUIRE(SurveyUsage(survey, 9759, 0) == Usage::Unknown);
}
TEST_CASE("Playerbot item usage preserves incomplete consumable and equipment inputs", "[playerbot][inventory]")
{
    REQUIRE(Consumable(PlayerbotConsumable::Usage::Unsupported) == Usage::Unknown);
    REQUIRE(Consumable(PlayerbotConsumable::Usage::None) == Usage::None);
    REQUIRE(Consumable(PlayerbotConsumable::Usage::Use) == Usage::Use);
    REQUIRE(Consumable(PlayerbotConsumable::Usage::Keep) == Usage::Keep);
    REQUIRE(Equipment(PlayerbotEquipment::Decision::Unknown) == Usage::Unknown);
    REQUIRE(Equipment(PlayerbotEquipment::Decision::NeedsRepair) == Usage::BrokenEquip);
    REQUIRE(Equipment(PlayerbotEquipment::Decision::ReplaceBroken) == Usage::Replace);
}
TEST_CASE("Playerbot carried usage requires matching entry and signed affix proof", "[playerbot][inventory]")
{
    PlayerbotEquipment::Survey survey;
    PlayerbotEquipment::Evaluation row;
    row.Input.Entry = 100; row.Input.Property = -8;
    row.Result = PlayerbotEquipment::Decision::FillSlot;
    survey.Items.push_back(row);
    REQUIRE(CarriedEquipment(survey, 100, -8) == Usage::Unknown);
    survey.Available = true;
    REQUIRE(CarriedEquipment(survey, 100, 8) == Usage::Unknown);
    REQUIRE(CarriedEquipment(survey, 101, -8) == Usage::Unknown);
    REQUIRE(CarriedEquipment(survey, 100, -8) == Usage::Equip);
    survey.Items.front().Result = PlayerbotEquipment::Decision::Keep;
    REQUIRE(CarriedEquipment(survey, 100, -8) == Usage::Keep);
    row.Result = PlayerbotEquipment::Decision::Unknown;
    survey.Items.push_back(row);
    REQUIRE(CarriedEquipment(survey, 100, -8) == Usage::Unknown);
    survey.Items.front().Result = PlayerbotEquipment::Decision::Upgrade;
    REQUIRE(CarriedEquipment(survey, 100, -8) == Usage::Equip);
}
TEST_CASE("Playerbot roll policy does not infer votes from unknown facts", "[playerbot][loot]")
{
    RollPolicy policy{2, true, true, true};
    REQUIRE_FALSE(ChooseRoll(Usage::Unknown, Kind::Equipment, policy, true, false, false));
    policy.NeedLevel = 3;
    REQUIRE_FALSE(ChooseRoll(Usage::Equip, Kind::Equipment, policy, true, false, false));
    REQUIRE(ChooseRoll(Usage::Equip, Kind::Equipment, {}, true, false, false) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::Equip, Kind::Equipment, {2, true, true, true}, false, false, false) == Vote::Pass);
}
TEST_CASE("Playerbot roll policy retains donor need uniqueness and downgrade rules", "[playerbot][loot]")
{
    REQUIRE(ChooseRoll(Usage::Equip, Kind::Equipment, {2, true, false, false}, true, false, false) == Vote::Need);
    REQUIRE(ChooseRoll(Usage::Replace, Kind::Equipment, {2, true, false, false}, true, true, false) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::Equip, Kind::Equipment, {1, false, false, false}, true, false, false) == Vote::Greed);
    REQUIRE(ChooseRoll(Usage::Keep, Kind::Equipment, {2, false, false, false}, true, false, false) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::Keep, Kind::Equipment, {2, true, false, false}, true, false, false) == Vote::Greed);
    REQUIRE(ChooseRoll(Usage::None, Kind::Equipment, {2, true, false, false}, true, false, false) == Vote::Pass);
}
TEST_CASE("Playerbot roll policy retains donor recipe and disenchant gates", "[playerbot][loot]")
{
    REQUIRE(ChooseRoll(Usage::Skill, Kind::Recipe, {2, true, false, false}, true, false, true) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::Skill, Kind::Recipe, {2, true, false, true}, true, false, true) == Vote::Need);
    REQUIRE(ChooseRoll(Usage::None, Kind::Recipe, {2, true, false, true}, true, false, true) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::None, Kind::Recipe, {2, true, false, true}, true, false, false) == Vote::Greed);
    REQUIRE(ChooseRoll(Usage::Disenchant, Kind::Equipment, {2, false, true, false}, true, false, false) == Vote::Disenchant);
    REQUIRE(ChooseRoll(Usage::Disenchant, Kind::Equipment, {2, true, false, false}, true, false, false) == Vote::Greed);
    REQUIRE(ChooseRoll(Usage::Disenchant, Kind::Equipment, {2, false, false, false}, true, false, false) == Vote::Pass);
    REQUIRE(ChooseRoll(Usage::Use, Kind::Other, {2, true, false, false}, true, false, false) == Vote::Greed);
    REQUIRE(ChooseRoll(Usage::Keep, Kind::Other, {2, true, false, false}, true, false, false) == Vote::Pass);
}
