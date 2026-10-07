/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotEquipmentApply.h"
#include <catch2/catch.hpp>
using namespace PlayerbotEquipmentApply;
TEST_CASE("Playerbot equip mailbox retains busy ownership through map completion", "[playerbot][inventory]")
{
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, 100));
    REQUIRE(mailbox.Post(1, 100));
    REQUIRE_FALSE(mailbox.Post(2, 101));
    auto request = mailbox.Take();
    REQUIRE(request);
    REQUIRE(request->Requester == 1);
    REQUIRE_FALSE(mailbox.Take());
    REQUIRE_FALSE(mailbox.Post(2, 102));
    mailbox.Finish(request->Serial + 1);
    REQUIRE_FALSE(mailbox.Post(2, 103));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(2, 104));
}
TEST_CASE("Playerbot equip intent expires at five seconds and handles timer wrap", "[playerbot][inventory]")
{
    Request request{1, 100, 1};
    REQUIRE(Fresh(request, 5099));
    REQUIRE_FALSE(Fresh(request, 5100));
    request.Created = UINT32_MAX - 100;
    REQUIRE(Fresh(request, 100));
    REQUIRE_FALSE(Fresh(request, 5000));
}
TEST_CASE("Playerbot equip execution considers only qualified safe decisions", "[playerbot][inventory]")
{
    using PlayerbotEquipment::Decision;
    REQUIRE(Actionable(Decision::Upgrade));
    REQUIRE(Actionable(Decision::FillSlot));
    REQUIRE(Actionable(Decision::ReplaceBroken));
    REQUIRE_FALSE(Actionable(Decision::Unknown));
    REQUIRE_FALSE(Actionable(Decision::Keep));
    REQUIRE_FALSE(Actionable(Decision::NeedsRepair));
}
