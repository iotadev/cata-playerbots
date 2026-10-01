/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#include "../src/Bot/Factory/PlayerbotAppearance.h"
#include <catch2/catch.hpp>

TEST_CASE("Factory appearance rejects incomplete and oversized data", "[playerbots][factory]")
{
    auto accepts = [](PlayerbotAppearance const&) { return true; };
    REQUIRE_FALSE(SelectPlayerbotAppearance({}, {{0, 0}}, {}, true, accepts));
    REQUIRE_FALSE(SelectPlayerbotAppearance({{0, 0}}, {}, {}, true, accepts));
    REQUIRE_FALSE(SelectPlayerbotAppearance({{0, 0}}, {{0, 0}}, {}, false, accepts));
    REQUIRE_FALSE(SelectPlayerbotAppearance(std::vector<PlayerbotAppearanceSection>(4097), {{0, 0}}, {}, true, accepts));
}

TEST_CASE("Factory appearance preserves face skin and hair facial color pairing", "[playerbots][factory]")
{
    auto selected = SelectPlayerbotAppearance({{3, 4}}, {{5, 6}}, {{7, 9}, {8, 6}}, false,
        [](PlayerbotAppearance const&) { return true; });
    REQUIRE(selected);
    REQUIRE(selected->Skin == 4);
    REQUIRE(selected->Face == 3);
    REQUIRE(selected->HairStyle == 5);
    REQUIRE(selected->HairColor == 6);
    REQUIRE(selected->FacialHair == 8);
}

TEST_CASE("Factory appearance skips native rejected choices", "[playerbots][factory]")
{
    auto selected = SelectPlayerbotAppearance({{0, 0}, {1, 2}}, {{0, 0}}, {}, true,
        [](PlayerbotAppearance const& value) { return value.Face == 1; });
    REQUIRE(selected);
    REQUIRE(selected->Skin == 2);
    REQUIRE(selected->FacialHair == 0);
    REQUIRE_FALSE(SelectPlayerbotAppearance({{0, 0}}, {{0, 0}}, {}, true,
        [](PlayerbotAppearance const&) { return false; }));
}

TEST_CASE("Factory appearance bounds native validation work", "[playerbots][factory]")
{
    std::size_t calls = 0;
    REQUIRE_FALSE(SelectPlayerbotAppearance(std::vector<PlayerbotAppearanceSection>(257),
        std::vector<PlayerbotAppearanceSection>(257), {}, true,
        [&calls](PlayerbotAppearance const&) { ++calls; return false; }));
    REQUIRE(calls == 65536);
}

TEST_CASE("Factory appearance rejects mismatched facial hair without native calls", "[playerbots][factory]")
{
    std::size_t calls = 0;
    REQUIRE_FALSE(SelectPlayerbotAppearance(std::vector<PlayerbotAppearanceSection>(257),
        std::vector<PlayerbotAppearanceSection>(257), {{0, 1}}, false,
        [&calls](PlayerbotAppearance const&) { ++calls; return true; }));
    REQUIRE(calls == 0);
}
