/* GPL v2 or later. See PORTING.md. */
#include "../src/Ai/Base/PlayerbotEquipmentInspection.h"
#include "../src/Bot/Cmd/PlayerbotAddonState.h"
#include <catch2/catch.hpp>
using namespace PlayerbotEquipmentInspection;
TEST_CASE("Playerbot gear inspection refresh is gated identity-bound and rate-limited", "[playerbot][inventory]")
{
    REQUIRE_FALSE(Due(false, false, false, 100, 0));
    REQUIRE(Due(true, false, true, 100, 0));
    REQUIRE(Due(true, true, false, 100, 0));
    REQUIRE_FALSE(Due(true, true, true, 1999, 0));
    REQUIRE(Due(true, true, true, 2000, 0));
    REQUIRE(Due(true, true, true, 2000, UINT32_MAX - 100));
}
TEST_CASE("Playerbot gear inspection freshness handles disable and timer wrap", "[playerbot][inventory]")
{
    REQUIRE(Fresh(true, true, 4999, 0));
    REQUIRE_FALSE(Fresh(true, true, 5000, 0));
    REQUIRE_FALSE(Fresh(false, true, 1, 0));
    REQUIRE_FALSE(Fresh(true, false, 1, 0));
    REQUIRE(Fresh(true, true, 100, UINT32_MAX - 100));
}
TEST_CASE("Playerbot gear inspection output is bounded and distinguishes unknown scores", "[playerbot][inventory]")
{
    PlayerbotEquipment::Survey survey;
    REQUIRE(Rows(survey).empty());
    survey.Available = true;
    for (int i = 0; i < 20; ++i)
    {
        PlayerbotEquipment::Evaluation item;
        item.Input.Entry = 100 + i;
        item.Result = PlayerbotEquipment::Decision::Unknown;
        survey.Items.push_back(item);
    }
    auto rows = Rows(survey);
    REQUIRE(rows.size() == 6);
    REQUIRE(rows.front().find("unknown/incomplete") != std::string::npos);
    REQUIRE(rows.front().find("?/?") != std::string::npos);
    survey.Items.front().CandidateScore = 12;
    survey.Items.front().ExistingScore = 10;
    survey.Items.front().Result = PlayerbotEquipment::Decision::Upgrade;
    REQUIRE(Rows(survey).front().find("12.00/10.00") != std::string::npos);
}
TEST_CASE("Playerbot gear-only publication cannot masquerade as ready strategy STATE", "[playerbot][inventory][addon]")
{
    PlayerbotStrategySnapshot snapshot;
    snapshot.Bot = 1; snapshot.Controller = 2; snapshot.Created = 100;
    snapshot.EquipmentEnabled = true; snapshot.EquipmentAvailable = true;
    REQUIRE_FALSE(PlayerbotAddonProtocol::SnapshotFresh(snapshot, 1, 2, 101));
    snapshot.StrategiesReady = true;
    REQUIRE(PlayerbotAddonProtocol::SnapshotFresh(snapshot, 1, 2, 101));
}
