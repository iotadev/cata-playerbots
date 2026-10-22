/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotRecipeMaterials.h"
#include <catch2/catch.hpp>
TEST_CASE("Recipe material facts keep professions distinct for the same item", "[playerbot][inventory][profession]")
{
    PlayerbotRecipeMaterials::Index index;
    REQUIRE(index.Add(164, 100));
    REQUIRE(index.Contains(164, 100));
    REQUIRE_FALSE(index.Contains(171, 100));
    REQUIRE(index.Add(171, 100));
    REQUIRE(index.Contains(171, 100));
    REQUIRE(index.Size() == 2);
    REQUIRE_FALSE(index.Contains(164, 101));
}
TEST_CASE("Recipe material bounds preserve positive facts without inventing negatives", "[playerbot][inventory][profession]")
{
    PlayerbotRecipeMaterials::Index index(1);
    REQUIRE_FALSE(index.Add(0, 100));
    REQUIRE_FALSE(index.Add(164, 0));
    REQUIRE(index.Add(164, 100));
    REQUIRE(index.Add(164, 100));
    REQUIRE_FALSE(index.Truncated);
    REQUIRE_FALSE(index.Add(171, 100));
    REQUIRE(index.Truncated);
    REQUIRE(index.Contains(164, 100));
    REQUIRE_FALSE(index.Contains(171, 100));
    REQUIRE(index.Size() == 1);
    PlayerbotRecipeMaterials::Index empty(0);
    REQUIRE_FALSE(empty.Add(164, 100));
    REQUIRE(empty.Truncated);
}
