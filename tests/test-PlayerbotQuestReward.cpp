/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotQuestReward.h"
#include <catch2/catch.hpp>
using namespace PlayerbotQuestReward;
using PlayerbotItemUsage::Usage;
TEST_CASE("Playerbot reward batch allows only zero or one well formed native choice", "[playerbot][quest]")
{
    std::array<uint32, 6> entries{}, counts{};
    REQUIRE(UnambiguousSlot(entries, counts) == 0);
    entries[5] = 100; counts[5] = 2;
    REQUIRE(UnambiguousSlot(entries, counts) == 5);
    entries[1] = 101; counts[1] = 1;
    REQUIRE_FALSE(UnambiguousSlot(entries, counts));
    entries[1] = 100; REQUIRE_FALSE(UnambiguousSlot(entries, counts));
    entries[1] = 0; REQUIRE_FALSE(UnambiguousSlot(entries, counts));
    counts[1] = 0; counts[5] = 0; REQUIRE_FALSE(UnambiguousSlot(entries, counts));
}
TEST_CASE("Playerbot manual reward item resolution preserves native sparse slots", "[playerbot][quest]")
{
    std::array<uint32, 6> entries{}, counts{};
    REQUIRE(ResolveItem(entries, counts, 0) == 0);
    REQUIRE_FALSE(ResolveItem(entries, counts, 100));
    entries[4] = 100; counts[4] = 2;
    REQUIRE(ResolveItem(entries, counts, 100) == 4);
    REQUIRE_FALSE(ResolveItem(entries, counts, 0));
    REQUIRE_FALSE(ResolveItem(entries, counts, 101));
}
TEST_CASE("Playerbot manual reward rejects ambiguous and malformed native choices", "[playerbot][quest]")
{
    std::array<uint32, 6> entries{}, counts{};
    entries[0] = entries[4] = 100; counts[0] = 1; counts[4] = 2;
    REQUIRE_FALSE(ResolveItem(entries, counts, 100));
    entries[4] = 0;
    REQUIRE_FALSE(ResolveItem(entries, counts, 100));
    counts[4] = 0; counts[0] = 0;
    REQUIRE_FALSE(ResolveItem(entries, counts, 100));
}
TEST_CASE("Playerbot reward choice preserves zero single and sparse native slots", "[playerbot][quest]")
{
    auto none = Select({});
    REQUIRE(none.Ready); REQUIRE(none.NoChoiceRequired); REQUIRE_FALSE(none.Preferred);
    auto single = Select({{4, 100, 2, Usage::Unknown, {}}});
    REQUIRE(single.Ready); REQUIRE(single.Preferred == 4);
}
TEST_CASE("Playerbot reward choice retains donor equipment replacement precedence", "[playerbot][quest]")
{
    auto ranked = Select({{0, 100, 1, Usage::Use, 100}, {1, 101, 1, Usage::BadEquip, 100},
                          {2, 102, 1, Usage::Equip, 20}, {4, 104, 1, Usage::Replace, 30}});
    REQUIRE(ranked.Ready);
    REQUIRE(ranked.Candidates == std::vector<uint8>{2, 4}); REQUIRE(ranked.Preferred == 4);
}
TEST_CASE("Playerbot reward choice keeps native order ties and never invents slot zero", "[playerbot][quest]")
{
    auto tied = Select({{4, 104, 1, Usage::Equip, 0}, {2, 102, 1, Usage::Equip, 0}});
    REQUIRE(tied.Ready); REQUIRE(tied.Preferred == 2);
    auto category = Select({{4, 104, 1, Usage::Quest, {}}, {1, 101, 1, Usage::Use, {}}});
    REQUIRE(category.Preferred == 1);
}
TEST_CASE("Playerbot reward choice refuses malformed unknown and incomplete score inputs", "[playerbot][quest]")
{
    REQUIRE_FALSE(Select({{6, 100, 1, Usage::Equip, 1}}).Ready);
    REQUIRE_FALSE(Select({{0, 0, 1, Usage::Equip, 1}}).Ready);
    REQUIRE_FALSE(Select({{0, 100, 0, Usage::Equip, 1}}).Ready);
    REQUIRE_FALSE(Select({{0, 100, 1, Usage::Equip, 1}, {0, 101, 1, Usage::Equip, 2}}).Ready);
    REQUIRE_FALSE(Select({{0, 100, 1, Usage::Equip, 1}, {1, 101, 1, Usage::Unknown, {}}}).Ready);
    auto missing = Select({{0, 100, 1, Usage::Equip, 1}, {1, 101, 1, Usage::Equip, {}}});
    REQUIRE(missing.Ready); REQUIRE(missing.ScoreIncomplete); REQUIRE_FALSE(missing.Preferred);
    auto nan = Select({{0, 100, 1, Usage::Equip, 1}, {1, 101, 1, Usage::Equip, std::numeric_limits<float>::quiet_NaN()}});
    REQUIRE(nan.ScoreIncomplete); REQUIRE_FALSE(nan.Preferred);
}
