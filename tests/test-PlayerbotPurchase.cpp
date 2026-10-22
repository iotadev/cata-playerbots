/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotPurchase.h"
#include <catch2/catch.hpp>
TEST_CASE("Vendor buying requires the exact named command rather than a failed numeric parse", "[playerbot][purchase]")
{
    REQUIRE(PlayerbotPurchase::VendorCommand("buy vendor"));
    for (auto text : {"buy", "buy 0", "buy vendor extra", "buy vendorx", "buy 159"})
        REQUIRE_FALSE(PlayerbotPurchase::VendorCommand(text));
    REQUIRE_FALSE(PlayerbotPurchase::VendorCommand(std::string_view("buy vendor\0x", 12)));
}
TEST_CASE("Vendor restock selects food and drink independently with stable level ordering", "[playerbot][purchase]")
{
    using namespace PlayerbotPurchase;
    Choices choices;
    Consider(choices, {117, 0, 5, Category::Food});
    Consider(choices, {159, 1, 5, Category::Drink});
    Consider(choices, {3770, 2, 25, Category::Food});
    Consider(choices, {3771, 3, 25, Category::Food});
    REQUIRE(choices[0]->Item == 3770);
    REQUIRE(choices[1]->Item == 159);
    Consider(choices, {3770, 1, 25, Category::Food});
    REQUIRE(choices[0]->Slot == 1);
}
TEST_CASE("Vendor restock ignores invalid or out of snapshot identities", "[playerbot][purchase]")
{
    using namespace PlayerbotPurchase;
    Choices choices;
    Consider(choices, {0, 0, 50, Category::Food});
    Consider(choices, {117, MaxVendorRows, 50, Category::Food});
    REQUIRE_FALSE(choices[0]); REQUIRE_FALSE(choices[1]);
    Consider(choices, {117, MaxVendorRows - 1, 5, Category::Food});
    REQUIRE(choices[0]->Item == 117);
}
TEST_CASE("Manual buy accepts one complete numeric item operand", "[playerbot][purchase]")
{
    REQUIRE(PlayerbotPurchase::Parse("buy 159") == 159);
    for (auto text : {"buy", "buy 0", "buy -1", "buy +159", "buy 159 5", "buy 4294967296", "buy vendor", "buy |Hitem:159|h", "buy 159x"})
        REQUIRE(PlayerbotPurchase::Parse(text) == 0);
    REQUIRE(PlayerbotPurchase::Parse(std::string_view("buy 159\0x", 9)) == 0);
}
TEST_CASE("Native vendor bundles fit bounded carried stock without overflow", "[playerbot][purchase]")
{
    REQUIRE(PlayerbotPurchase::BundleFits(5, 20, 35));
    REQUIRE_FALSE(PlayerbotPurchase::BundleFits(5, 20, 36));
    REQUIRE_FALSE(PlayerbotPurchase::BundleFits(21, 20, 0));
    REQUIRE_FALSE(PlayerbotPurchase::BundleFits(0, 20, 0));
    REQUIRE_FALSE(PlayerbotPurchase::BundleFits(5, 0, 0));
    REQUIRE_FALSE(PlayerbotPurchase::BundleFits(5, 20, 0xffffffffu));
}
TEST_CASE("Native purchase intent keeps its item through the shared mailbox", "[playerbot][purchase]")
{
    PlayerbotPurchase::Mailbox mailbox;
    REQUIRE(mailbox.Post(3, 5, 530, 0, 10, 159));
    auto request = mailbox.Take(); REQUIRE(request);
    REQUIRE(request->Item == 159);
    REQUIRE_FALSE(mailbox.Post(3, 5, 530, 0, 11, 159));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(3, 5, 530, 0, 12, 117));
}
TEST_CASE("Vendor recovery supplies keep independent category choices and reject unsupported buckets", "[playerbot][purchase]")
{
    using namespace PlayerbotPurchase;
    Choices choices;
    Consider(choices, {118, 0, 5, Category::HealingPotion});
    Consider(choices, {858, 1, 15, Category::HealingPotion});
    Consider(choices, {2455, 2, 15, Category::ManaPotion});
    Consider(choices, {1251, 3, 5, Category::Bandage});
    REQUIRE(choices[uint32(Category::HealingPotion)]->Item == 858);
    REQUIRE(choices[uint32(Category::ManaPotion)]->Item == 2455);
    REQUIRE(choices[uint32(Category::Bandage)]->Item == 1251);
    REQUIRE_FALSE(choices[uint32(Category::Food)]);
    REQUIRE_FALSE(choices[uint32(Category::Drink)]);
    Consider(choices, {999, 4, 100, Category::Unsupported});
    Consider(choices, {999, 4, 100, static_cast<Category>(99)});
    REQUIRE(choices[uint32(Category::Bandage)]->Item == 1251);
}
