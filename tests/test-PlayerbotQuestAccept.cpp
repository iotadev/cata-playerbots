/* GPL v2 or later. See AUTHORS.md and PORTING.md. */
#include "../src/Ai/Base/PlayerbotQuestAccept.h"
#include <catch2/catch.hpp>
using namespace PlayerbotQuestAccept;
TEST_CASE("Playerbot reward batch requires a giver and preserves bounded mailbox admission", "[playerbot][quest]")
{
    REQUIRE(Valid(Operation::RewardAll, 0, 0));
    REQUIRE_FALSE(Valid(Operation::RewardAll, 100, 0));
    REQUIRE_FALSE(Valid(Operation::RewardAll, 0, 9758));
    REQUIRE_FALSE(IsInspection(Operation::RewardAll));
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(1, 0, 0, 530, 0, 1, Operation::RewardAll));
    REQUIRE(mailbox.Post(1, 0, 200, 530, 0, 1, Operation::RewardAll));
    auto request = mailbox.Take(); REQUIRE(request); REQUIRE(request->Action == Operation::RewardAll);
    REQUIRE_FALSE(mailbox.Post(1, 0, 200, 530, 0, 2, Operation::RewardAll));
    mailbox.Finish(request->Serial); REQUIRE(mailbox.Post(1, 0, 200, 530, 0, 2, Operation::RewardAll));
}
TEST_CASE("Playerbot quest listing preserves donor filters without changing default output", "[playerbot][quest]")
{
    REQUIRE(ParseInspection("quests") == Operation::Inspect);
    REQUIRE(ParseInspection("quests all") == Operation::Inspect);
    for (auto text : {"quests completed", "quests co"}) REQUIRE(ParseInspection(text) == Operation::InspectCompleted);
    for (auto text : {"quests incompleted", "quests in"}) REQUIRE(ParseInspection(text) == Operation::InspectIncompleted);
    REQUIRE(ParseInspection("quests summary") == Operation::InspectSummary);
    for (auto text : {"quests travel", "quests all extra", "quests 1", "quests incomplete", "questscompleted"})
        REQUIRE_FALSE(ParseInspection(text));
    REQUIRE(ShowQuest(Operation::Inspect, true)); REQUIRE(ShowQuest(Operation::Inspect, false));
    REQUIRE(ShowQuest(Operation::InspectCompleted, true)); REQUIRE_FALSE(ShowQuest(Operation::InspectCompleted, false));
    REQUIRE(ShowQuest(Operation::InspectIncompleted, false)); REQUIRE_FALSE(ShowQuest(Operation::InspectIncompleted, true));
    REQUIRE_FALSE(ShowQuest(Operation::InspectSummary, true)); REQUIRE_FALSE(ShowQuest(Operation::InspectSummary, false));
}
TEST_CASE("Playerbot quest list filters retain read-only zero operands and mailbox fencing", "[playerbot][quest]")
{
    for (auto action : {Operation::Inspect, Operation::InspectCompleted, Operation::InspectIncompleted, Operation::InspectSummary})
    {
        REQUIRE(IsInspection(action)); REQUIRE(Valid(action, 0, 0));
        REQUIRE_FALSE(Valid(action, 100, 0)); REQUIRE_FALSE(Valid(action, 0, 9758));
        Mailbox mailbox;
        REQUIRE(mailbox.Post(1, 0, 0, 530, 0, 1, action));
        auto request = mailbox.Take(); REQUIRE(request); REQUIRE(request->Action == action);
        REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 2));
        mailbox.Finish(request->Serial); REQUIRE(mailbox.Post(1, 0, 0, 530, 0, 2, action));
    }
    REQUIRE_FALSE(IsInspection(Operation::AcceptAll)); REQUIRE_FALSE(IsInspection(static_cast<Operation>(999)));
    REQUIRE_FALSE(ShowQuest(Operation::Reward, true));
}
TEST_CASE("Playerbot accept all requires a giver and zero single quest intent", "[playerbot][quest]")
{
    REQUIRE(Valid(Operation::AcceptAll, 0, 0));
    REQUIRE_FALSE(Valid(Operation::AcceptAll, 100, 0));
    REQUIRE_FALSE(Valid(Operation::AcceptAll, 0, 9758));
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(1, 0, 0, 530, 0, 1, Operation::AcceptAll));
    REQUIRE(mailbox.Post(1, 0, 200, 530, 0, 1, Operation::AcceptAll));
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 1));
}
TEST_CASE("Playerbot quest drop is one explicit active quest intent without reward data", "[playerbot][quest]")
{
    REQUIRE(ParseDrop("drop 9193") == 9193);
    REQUIRE(RecognizesDrop("drop"));
    REQUIRE_FALSE(RecognizesDrop("dropped 9193"));
    for (auto text : {"drop", "drop 0", "drop *", "drop -1", "drop 1 2", "drop 4294967296"})
        REQUIRE(ParseDrop(text) == 0);
    REQUIRE(Valid(Operation::Abandon, 9193, 0));
    REQUIRE_FALSE(Valid(Operation::Abandon, 0, 0));
    REQUIRE_FALSE(Valid(Operation::Abandon, 9193, 9758));
}
TEST_CASE("Playerbot quest abandonment is fenced against a second queued operation", "[playerbot][quest]")
{
    Mailbox mailbox;
    REQUIRE(mailbox.Post(1, 9193, 0, 530, 0, 1, Operation::Abandon));
    auto request = mailbox.Take();
    REQUIRE(request); REQUIRE(request->Action == Operation::Abandon); REQUIRE(request->Giver == 0);
    REQUIRE_FALSE(mailbox.Post(1, 9193, 0, 530, 0, 2, Operation::Abandon));
    mailbox.Finish(request->Serial + 1);
    REQUIRE_FALSE(mailbox.Post(1, 0, 0, 530, 0, 2, Operation::Inspect));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(1, 0, 0, 530, 0, 2, Operation::Inspect));
}
TEST_CASE("Playerbot outgoing quest share requires one native positive quest ID", "[playerbot][quest]")
{
    REQUIRE(ParseShare("share 9193") == 9193);
    REQUIRE(RecognizesShare("share"));
    REQUIRE_FALSE(RecognizesShare("shared 9193"));
    for (auto text : {"share", "share 0", "share -1", "share *", "share 1 2", "share 2147483648", "share  9193"})
        REQUIRE(ParseShare(text) == 0);
    REQUIRE(Valid(Operation::Share, 9193, 0));
    REQUIRE_FALSE(Valid(Operation::Share, 9193, 9758));
}
TEST_CASE("Playerbot outgoing quest share uses no giver and cannot overwrite another operation", "[playerbot][quest]")
{
    Mailbox mailbox;
    REQUIRE(mailbox.Post(1, 9193, 0, 530, 0, 1, Operation::Share));
    auto request = mailbox.Take();
    REQUIRE(request); REQUIRE(request->Action == Operation::Share); REQUIRE(request->Quest == 9193);
    REQUIRE(request->Giver == 0);
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 2));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(1, 100, 200, 530, 0, 2));
}
TEST_CASE("Playerbot quest inspection has no quest giver or reward mutation intent", "[playerbot][quest]")
{
    REQUIRE(Valid(Operation::Inspect, 0, 0));
    REQUIRE_FALSE(Valid(Operation::Inspect, 100, 0));
    REQUIRE_FALSE(Valid(Operation::Inspect, 0, 9758));
    REQUIRE_FALSE(Valid(Operation::Accept, 0, 0));
    REQUIRE_FALSE(Valid(Operation::Reward, 0, 0));
    Mailbox mailbox;
    REQUIRE(mailbox.Post(1, 0, 0, 530, 7, 1, Operation::Inspect, 0));
    auto request = mailbox.Take();
    REQUIRE(request); REQUIRE(request->Action == Operation::Inspect);
    REQUIRE(request->Giver == 0); REQUIRE(request->Item == 0);
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 7, 1));
}
TEST_CASE("Playerbot explicit reward parses item intent without accepting signed quest overflow", "[playerbot][quest]")
{
    auto args = ParseReward("reward 9193 9758");
    REQUIRE(args); REQUIRE(args->Quest == 9193); REQUIRE(args->Item == 9758);
    REQUIRE(ParseReward("reward 9193 0"));
    for (auto text : {"reward", "reward 9193", "reward 0 1", "reward 2147483648 1",
        "reward 9193 -1", "reward 9193 1 2", "reward 9193 4294967296", "reward 9193 ", "reward  9193 1"})
        REQUIRE_FALSE(ParseReward(text));
}
TEST_CASE("Playerbot quest command mailbox fences acceptance against reward intent", "[playerbot][quest]")
{
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 1, Operation::Accept, 9758));
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 1, static_cast<Operation>(9), 0));
    REQUIRE(mailbox.Post(1, 100, 200, 530, 0, 1, Operation::Reward, 9758));
    auto request = mailbox.Take();
    REQUIRE(request); REQUIRE(request->Action == Operation::Reward); REQUIRE(request->Item == 9758);
    REQUIRE_FALSE(mailbox.Post(1, 100, 200, 530, 0, 1));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(1, 100, 200, 530, 0, 1));
}
TEST_CASE("Playerbot quest accept parser requires one bounded numeric quest", "[playerbot][quest]")
{
    REQUIRE(Parse("accept 9193") == 9193);
    REQUIRE(Recognizes("accept"));
    REQUIRE_FALSE(Recognizes("accepting 9193"));
    for (auto invalid : {"accept", "accept *", "accept 0", "accept -1", "accept +1", "accept 1 2",
         "accept 4294967296", "accept 9193x", "accept  9193", "accept |Hquest:9193|h"})
        REQUIRE(Parse(invalid) == 0);
}
TEST_CASE("Playerbot quest accept mailbox retains copied giver and instance until completion", "[playerbot][quest]")
{
    Mailbox mailbox;
    REQUIRE_FALSE(mailbox.Post(0, 100, 200, 530, 0, 10));
    REQUIRE_FALSE(mailbox.Post(1, 0, 200, 530, 0, 10));
    REQUIRE_FALSE(mailbox.Post(1, 100, 0, 530, 0, 10));
    REQUIRE(mailbox.Post(1, 100, 200, 530, 7, 10));
    auto request = mailbox.Take();
    REQUIRE(request);
    REQUIRE(request->Giver == 200);
    REQUIRE(request->Map == 530);
    REQUIRE(request->Instance == 7);
    REQUIRE_FALSE(mailbox.Take());
    REQUIRE_FALSE(mailbox.Post(1, 101, 201, 530, 7, 11));
    mailbox.Finish(request->Serial + 1);
    REQUIRE_FALSE(mailbox.Post(1, 101, 201, 530, 7, 11));
    mailbox.Finish(request->Serial);
    REQUIRE(mailbox.Post(1, 101, 201, 530, 7, 11));
}
TEST_CASE("Playerbot quest accept intent expires across timer wrap", "[playerbot][quest]")
{
    Request request;
    request.Created = uint32(-100);
    REQUIRE(Fresh(request, 0));
    REQUIRE(Fresh(request, 4899));
    REQUIRE_FALSE(Fresh(request, 4900));
}
