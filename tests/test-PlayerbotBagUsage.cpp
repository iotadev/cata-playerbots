/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotBagUsage.h"
#include "../src/Ai/Base/PlayerbotQuestReward.h"
#include <catch2/catch.hpp>
TEST_CASE("Bag capacity retention includes the first carried bag slot and empty slots", "[playerbot][bags]")
{
    using namespace PlayerbotBagUsage;
    std::array<Slot, 4> slots = {Slot{true, true, 6}, Slot{true, true, 16}, Slot{true, true, 16}, Slot{true, true, 16}};
    REQUIRE(CapacityUpgrade(8, true, slots));
    slots[0] = {true, true, 16};
    REQUIRE_FALSE(CapacityUpgrade(8, true, slots));
    slots[3] = {true, true, 0};
    REQUIRE(CapacityUpgrade(6, true, slots));
    REQUIRE_FALSE(CapacityUpgrade(6, false, slots));
}
TEST_CASE("Bag retention protects specialized and unresolved destinations", "[playerbot][bags]")
{
    using namespace PlayerbotBagUsage;
    std::array<Slot, 4> slots{};
    REQUIRE_FALSE(CapacityUpgrade(16, true, slots));
    slots[0] = {true, false, 4};
    REQUIRE_FALSE(CapacityUpgrade(16, true, slots));
    slots[1] = {true, true, 99};
    REQUIRE_FALSE(CapacityUpgrade(16, true, slots));
    slots[2] = {true, true, 16};
    REQUIRE_FALSE(CapacityUpgrade(16, true, slots));
    REQUIRE_FALSE(CapacityUpgrade(0, true, slots));
    REQUIRE_FALSE(CapacityUpgrade(MaxCapacity + 1, true, slots));
    REQUIRE(CapacityUpgrade(20, true, slots));
}
TEST_CASE("Bag upgrade facts join reward ranking without inventing cross-item scores", "[playerbot][bags][quest]")
{
    using namespace PlayerbotQuestReward;
    using PlayerbotItemUsage::Usage;
    auto ranked = Select({Choice{0, 100, 1, Usage::Equip, {}}, Choice{3, 200, 1, Usage::Keep, {}}});
    REQUIRE(ranked.Ready);
    REQUIRE(ranked.Preferred == 0);
    auto tied = Select({Choice{0, 100, 1, Usage::Equip, {}}, Choice{3, 200, 1, Usage::Equip, {}}});
    REQUIRE(tied.Ready);
    REQUIRE(tied.ScoreIncomplete);
    REQUIRE_FALSE(tied.Preferred);
}
TEST_CASE("Explicit bag fill requires one empty ordinary owned backpack bag", "[playerbot][bags][inventory]")
{
    using PlayerbotBagUsage::SafeCarriedBag;
    REQUIRE(SafeCarriedBag(true, true, true, true, false, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(false, true, true, true, false, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(true, false, true, true, false, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, false, true, false, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, true, false, false, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, true, true, true, 1, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, true, true, false, 0, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, true, true, false, 2, false));
    REQUIRE_FALSE(SafeCarriedBag(true, true, true, true, false, 1, true));
}
TEST_CASE("Bag destination prefers empty slots then the smallest proven ordinary bag", "[playerbot][bags][inventory]")
{
    using namespace PlayerbotBagUsage;
    std::array<Slot, 4> slots = {Slot{true, true, 8}, Slot{true, true, 6}, Slot{true, true, 6}, Slot{true, false, 0}};
    REQUIRE(Destination(10, true, slots) == 1);
    slots[2] = {true, true, 0};
    REQUIRE(Destination(10, true, slots) == 2);
    REQUIRE_FALSE(Destination(10, false, slots));
}
TEST_CASE("Native bag content confirmation preserves identities quantities and affixes", "[playerbot][bags][inventory]")
{
    using namespace PlayerbotBagUsage;
    std::vector<Content> original = {{1, 100, 3, -4, 12}, {2, 200, 1, 0, 0}};
    REQUIRE(SameContents(original, original));
    auto changed = original; changed[0].Guid = 99; REQUIRE_FALSE(SameContents(original, changed));
    changed = original; changed[0].Count = 2; REQUIRE_FALSE(SameContents(original, changed));
    changed = original; changed[0].Property = -5; REQUIRE_FALSE(SameContents(original, changed));
    changed = original; changed[0].Factor = 13; REQUIRE_FALSE(SameContents(original, changed));
    changed = original; changed.pop_back(); REQUIRE_FALSE(SameContents(original, changed));
    changed = original; changed.push_back({3, 300, 1, 0, 0}); REQUIRE_FALSE(SameContents(original, changed));
}
