/* GPL v2 or later. */
#include "../src/Ai/Base/PlayerbotTraining.h"
#include <catch2/catch.hpp>
TEST_CASE("Class training accepts one positive native signed spell operand", "[playerbot][training]")
{
    REQUIRE(PlayerbotTraining::Parse("train 100") == 100);
    REQUIRE(PlayerbotTraining::Parse("train 2147483647") == 2147483647);
    for (auto text : {"train", "train 0", "train -1", "train +100", "train 100 extra", "train 2147483648", "train 4294967296", "train all", "train |Hspell:100|h", "train 100x"})
        REQUIRE(PlayerbotTraining::Parse(text) == 0);
    REQUIRE(PlayerbotTraining::Parse(std::string_view("train 100\0x", 11)) == 0);
    REQUIRE(PlayerbotTraining::Recognizes("train"));
    REQUIRE(PlayerbotTraining::Recognizes("train all"));
    REQUIRE(PlayerbotTraining::Recognizes("trainer"));
    REQUIRE_FALSE(PlayerbotTraining::Recognizes("trainers"));
}
TEST_CASE("Trainer inspection and learning batch cannot come from a failed numeric parse", "[playerbot][training]")
{
    using namespace PlayerbotTraining;
    REQUIRE(ParseRequest("trainer") == Inspect);
    REQUIRE(ParseRequest("train ?") == Inspect);
    REQUIRE(ParseRequest("trainer learn") == LearnAll);
    REQUIRE(ParseRequest("train all") == LearnAll);
    REQUIRE(ParseRequest("train 100") == 100);
    for (auto text : {"train", "train 0", "train 4294967295", "train all extra", "trainer learn extra", "trainerx", "trainer ?", "train -1"})
        REQUIRE_FALSE(ParseRequest(text));
    REQUIRE_FALSE(ParseRequest(std::string_view("trainer\0x", 9)));
    REQUIRE(MaxListed <= MaxOffers);
    REQUIRE(MaxPurchases <= MaxOffers);
}
