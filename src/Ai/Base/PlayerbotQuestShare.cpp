/* GPL v2 or later. Donor behavior and Cata adaptations are in PORTING.md. */
#include "PlayerbotQuestShare.h"
#include "../../Script/PlayerbotConfig.h"
#include "PlayerbotSecurity.h"
#include "Chat.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include "WorldSession.h"

void PlayerbotQuestShare::Update(WorldSession& session, Attempt& attempt)
{
    Player* bot = session.GetPlayer();
    if (!bot || !session.IsServerOrigin()) { attempt.Reset(); return; }
    ObjectGuid sharerGuid = bot->GetPlayerSharingQuest();
    uint32 questId = bot->GetSharedQuestID();
    if (sharerGuid.IsEmpty() || !questId) { attempt.Reset(); return; }
    if (!PlayerbotModuleQuestShareEnabled() || session.PlayerLoading() || session.isLogingOut() ||
        !bot->IsInWorld() || !bot->IsAlive() || bot->IsInFlight() ||
        bot->IsBeingTeleported() || bot->IsInCombat() || bot->IsNonMeleeSpellCast(false)) return;
    // Resolve current native identity on the world owner, like the core's share
    // handler; do not use a map-owned AI/controller pointer from this context.
    Player* sharer = ObjectAccessor::FindConnectedPlayer(sharerGuid);
    if (!sharer || !sharer->GetSession() || sharer->GetSession()->IsServerOrigin() ||
        sharer->GetSession()->isLogingOut() ||
        !sharer->IsInWorld() || !sharer->IsAlive() || sharer->IsBeingTeleported() ||
        sharer->GetMap() != bot->GetMap() || !bot->GetGroup() ||
        sharer->GetGroup() != bot->GetGroup()) return;
    uint32 controller = sharerGuid.GetCounter();
    if ((session.GetServerOriginFollowTargetGuidLow() != controller &&
         session.GetServerOriginPartyControllerGuidLow() != controller) ||
        !PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *sharer)) return;
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest) return;
    auto route = ChooseRoute(questId, quest->IsTurnIn(), quest->IsPushedToPartyOnAccept(),
        sharer->IsActiveQuest(questId), sharer->CanShareQuest(questId));
    if (route == Route::Unavailable) return;
    if (!attempt.Take(sharerGuid.GetRawValue(), questId)) return;

    // Typed native operation, not a queued WotLK wire packet or direct AddQuest.
    // The handler retains interaction, sharing, eligibility, capacity, source
    // item/spell and quest-script ownership. Admission does not prove success.
    bool alreadyPresent = bot->FindQuestSlot(questId) < MAX_QUEST_LOG_SIZE;
    if (route == Route::PartyConfirmation)
    {
        WorldPackets::Quest::QuestConfirmAccept packet(WorldPacket(CMSG_QUEST_CONFIRM_ACCEPT, 0));
        packet.QuestID = int32(questId);
        session.HandleQuestConfirmAccept(packet);
    }
    else
    {
        WorldPackets::Quest::QuestGiverAcceptQuest packet(WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST, 0));
        packet.QuestGiverGUID = sharerGuid;
        packet.QuestID = questId;
        packet.StartCheat = 0;
        session.HandleQuestgiverAcceptQuestOpcode(packet);
    }
    bool accepted = !alreadyPresent && bot->FindQuestSlot(questId) < MAX_QUEST_LOG_SIZE;
    char const* outcome = alreadyPresent ? "already present" : accepted ? "accepted" : "not accepted";
    TC_LOG_INFO("module.playerbots", "PB-QUEST: %s %s share %u from %s: %s",
        bot->GetName().c_str(), route == Route::PartyConfirmation ? "party-confirmation" : "ordinary",
        questId, sharer->GetName().c_str(), outcome);
    ChatHandler(sharer->GetSession()).PSendSysMessage("Playerbot %s: shared quest %u %s.",
        bot->GetName().c_str(), questId, alreadyPresent ? "already present" :
            accepted ? "accepted" : "not accepted by native checks");
    if (bot->GetPlayerSharingQuest().IsEmpty()) attempt.Reset();
}
