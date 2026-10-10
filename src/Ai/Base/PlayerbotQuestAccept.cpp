/* GPL v2 or later. Donor/Cata differences are in PORTING.md. */
#include "PlayerbotQuestAccept.h"
#include "PlayerbotQuestReward.h"
#include "../../Script/PlayerbotConfig.h"
#include "PlayerbotSecurity.h"
#include "Chat.h"
#include "Group.h"
#include "GossipDef.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "QuestDef.h"
#include "QuestPackets.h"
#include "Timer.h"
#include "WorldSession.h"
#include "Util.h"
#include <algorithm>
#include <sstream>

namespace
{
// Donor ListQuestsAction semantics, limited to native active-log facts. Counts
// are observations, not a simulated progress/completion or travel model.
void ReportQuests(Player& bot, Player& requester, PlayerbotQuestAccept::Operation action)
{
    ChatHandler reply(requester.GetSession());
    uint32 total = 0, complete = 0, incomplete = 0, failed = 0;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 id = bot.GetQuestSlotQuestId(slot);
        if (!id) continue;
        ++total;
        auto status = bot.GetQuestStatus(id);
        complete += status == QUEST_STATUS_COMPLETE;
        incomplete += status == QUEST_STATUS_INCOMPLETE;
        failed += status == QUEST_STATUS_FAILED;
        // Donor's incomplete filter means not complete, including failed entries.
        if (!PlayerbotQuestAccept::ShowQuest(action, status == QUEST_STATUS_COMPLETE)) continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(id);
        if (!quest) { reply.PSendSysMessage("Playerbot %s: quest %u metadata unavailable.", bot.GetName().c_str(), id); continue; }
        std::string title = quest->GetTitle(); utf8truncate(title, 64);
        std::ostringstream text;
        text << "Playerbot " << bot.GetName() << ": quest " << id << " " << title << " ["
             << (status == QUEST_STATUS_COMPLETE ? "complete" : status == QUEST_STATUS_INCOMPLETE ? "incomplete" :
                 status == QUEST_STATUS_FAILED ? "failed" : "other") << "]";
        for (uint8 objective = 0; objective < QUEST_ITEM_OBJECTIVES_COUNT; ++objective)
            if (quest->RequiredItemId[objective])
                text << "; carried " << quest->RequiredItemId[objective] << "="
                     << bot.GetItemCount(quest->RequiredItemId[objective], false) << "/" << quest->RequiredItemCount[objective];
        for (uint8 objective = 0; objective < QUEST_OBJECTIVES_COUNT; ++objective)
            if (quest->RequiredNpcOrGo[objective])
                text << "; target " << quest->RequiredNpcOrGo[objective] << "="
                     << bot.GetQuestSlotCounter(slot, objective) << "/" << quest->RequiredNpcOrGoCount[objective];
        bool choices = false;
        for (uint8 choice = 0; choice < QUEST_REWARD_CHOICES_COUNT; ++choice)
            if (quest->RewardChoiceItemId[choice])
            {
                choices = true;
                text << "; reward item " << quest->RewardChoiceItemId[choice] << "x" << quest->RewardChoiceItemCount[choice];
            }
        if (!choices) text << "; no choice item (reward item 0)";
        reply.SendSysMessage(text.str().c_str());
    }
    reply.PSendSysMessage("Playerbot %s quests: %u/25 active, %u incomplete, %u complete, %u failed. Read-only; other objective types are not detailed.",
        bot.GetName().c_str(), total, incomplete, complete, failed);
}
}

void PlayerbotQuestAccept::Update(WorldSession& session, Mailbox& mailbox)
{
    auto request = mailbox.Take();
    if (!request) return;
    Player* bot = session.GetPlayer();
    Player* requester = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
    char const* outcome = "rejected; authority, state or giver was not eligible";
    bool attached = session.GetServerOriginFollowTargetGuidLow() == request->Requester ||
        session.GetServerOriginPartyControllerGuidLow() == request->Requester;
    bool rewardAll = request->Action == Operation::RewardAll;
    bool reward = request->Action == Operation::Reward || rewardAll;
    bool inspect = IsInspection(request->Action);
    bool share = request->Action == Operation::Share;
    bool abandon = request->Action == Operation::Abandon;
    bool acceptAll = request->Action == Operation::AcceptAll;
    bool inspectionReported = false;
    bool enabled = abandon ? PlayerbotModuleQuestAbandonEnabled() : share ? PlayerbotModuleQuestSendShareEnabled() : inspect ? PlayerbotModuleQuestInspectionEnabled() : reward ? PlayerbotModuleQuestRewardEnabled() : PlayerbotModuleQuestAcceptEnabled();
    if (enabled && Valid(request->Action, request->Quest, request->Item) && Fresh(*request, getMSTime()) && session.IsServerOrigin() &&
        !session.PlayerLoading() && !session.isLogingOut() && bot && bot->IsInWorld() && (inspect || bot->IsAlive()) &&
        !bot->IsBeingTeleported() && (inspect || (!bot->IsInCombat() && !bot->IsInFlight() && !bot->IsNonMeleeSpellCast(false))) &&
        (inspect || bot->GetPlayerSharingQuest().IsEmpty()) && attached && requester && requester->GetSession() &&
        !requester->GetSession()->IsServerOrigin() && !requester->GetSession()->isLogingOut() &&
        requester->IsInWorld() && (inspect || requester->IsAlive()) && !requester->IsBeingTeleported() &&
        (inspect || !requester->IsInCombat()) && requester->GetMap() == bot->GetMap() &&
        bot->GetMapId() == request->Map && bot->GetInstanceId() == request->Instance &&
        PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *requester))
    {
        if (inspect) { ReportQuests(*bot, *requester, request->Action); inspectionReported = true; outcome = "inspection completed; no quests changed"; }
        if (abandon)
        {
            uint16 slot = bot->FindQuestSlot(request->Quest);
            if (slot < MAX_QUEST_LOG_SIZE && bot->GetQuestSlotQuestId(slot) == request->Quest)
            {
                WorldPackets::Quest::QuestLogRemoveQuest packet(WorldPacket(CMSG_QUEST_LOG_REMOVE_QUEST, 0));
                packet.Entry = uint8(slot);
                session.HandleQuestLogRemoveQuest(packet);
                outcome = bot->FindQuestSlot(request->Quest) >= MAX_QUEST_LOG_SIZE ? "abandonment confirmed" : "abandonment rejected by native checks";
            }
            else outcome = "abandonment rejected; quest is not in the active log";
        }
        if (share)
        {
            if (bot->GetGroup() && bot->GetGroup() == requester->GetGroup() && !bot->GetGroup()->isBGGroup() &&
                bot->FindQuestSlot(request->Quest) < MAX_QUEST_LOG_SIZE && bot->CanShareQuest(request->Quest))
            {
                // Use this Cata handler's actual uint32 input; do not copy the
                // donor typed packet or manufacture recipient sharing state.
                WorldPacket packet(CMSG_PUSHQUESTTOPARTY, 4);
                packet << request->Quest;
                session.HandlePushQuestToParty(packet);
                outcome = "party share submitted; recipient acceptance not confirmed";
            }
            else outcome = "share rejected; current party or native share eligibility missing";
        }
        ObjectGuid giver(request->Giver);
        Object* object = (inspect || share || abandon) ? nullptr : ObjectAccessor::GetObjectByTypeMask(*bot, giver, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
        if (!inspect && !share && !abandon && (giver.IsCreature() || giver.IsGameObject()) && object &&
            (acceptAll || rewardAll || (reward ? object->hasInvolvedQuest(request->Quest) : object->hasQuest(request->Quest))) &&
            requester->CanInteractWithQuestGiver(object) && bot->CanInteractWithQuestGiver(object))
        {
            if (acceptAll || rewardAll)
            {
                std::vector<uint32> candidates;
                if (acceptAll)
                {
                    bot->PrepareQuestMenu(giver);
                    for (auto const& row : bot->PlayerTalkClass->GetQuestMenu().GetQuestMenuItems())
                        if (row.QuestIcon == 2 && candidates.size() < MAX_QUEST_LOG_SIZE)
                            candidates.push_back(row.QuestId);
                }
                else
                    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
                        if (uint32 id = bot->GetQuestSlotQuestId(slot)) candidates.push_back(id);
                uint32 accepted = 0, attempted = 0;
                // Native callbacks can replace menus, start casts or transfer a
                // player. Retain IDs only, resolve/revalidate on every iteration.
                for (uint32 id : candidates)
                {
                    Player* currentBot = session.GetPlayer();
                    Player* currentRequester = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
                    bool currentAttachment = session.GetServerOriginFollowTargetGuidLow() == request->Requester || session.GetServerOriginPartyControllerGuidLow() == request->Requester;
                    if (!(rewardAll ? PlayerbotModuleQuestRewardEnabled() : PlayerbotModuleQuestAcceptEnabled()) || !Fresh(*request, getMSTime()) || !currentBot || !currentRequester ||
                        session.isLogingOut() || !currentBot->IsInWorld() || !currentBot->IsAlive() || currentBot->IsBeingTeleported() ||
                        currentBot->IsInCombat() || currentBot->IsInFlight() || currentBot->IsNonMeleeSpellCast(false) ||
                        !currentBot->GetPlayerSharingQuest().IsEmpty() || !currentRequester->GetSession() ||
                        currentRequester->GetSession()->IsServerOrigin() || currentRequester->GetSession()->isLogingOut() ||
                        !currentRequester->IsInWorld() || !currentRequester->IsAlive() || currentRequester->IsBeingTeleported() ||
                        currentRequester->IsInCombat() || currentRequester->GetMap() != currentBot->GetMap() ||
                        currentBot->GetMapId() != request->Map || currentBot->GetInstanceId() != request->Instance ||
                        !currentAttachment || !PlayerbotSecurity(*currentBot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *currentRequester)) break;
                    Object* currentGiver = ObjectAccessor::GetObjectByTypeMask(*currentBot, giver, TYPEMASK_UNIT | TYPEMASK_GAMEOBJECT);
                    if (!currentGiver || !currentBot->CanInteractWithQuestGiver(currentGiver) || !currentRequester->CanInteractWithQuestGiver(currentGiver)) break;
                    Quest const* quest = sObjectMgr->GetQuestTemplate(id);
                    if (rewardAll)
                    {
                        if (!quest || id > uint32(std::numeric_limits<int32>::max()) || quest->IsRepeatable() || quest->IsTurnIn() ||
                            !currentGiver->hasInvolvedQuest(id) || currentBot->FindQuestSlot(id) >= MAX_QUEST_LOG_SIZE ||
                            currentBot->GetQuestStatus(id) != QUEST_STATUS_COMPLETE || currentBot->GetQuestRewardStatus(id)) continue;
                        std::array<uint32, 6> entries{}, counts{};
                        static_assert(QUEST_REWARD_CHOICES_COUNT == 6);
                        std::copy_n(quest->RewardChoiceItemId, 6, entries.begin());
                        std::copy_n(quest->RewardChoiceItemCount, 6, counts.begin());
                        auto slot = PlayerbotQuestReward::UnambiguousSlot(entries, counts);
                        if (!slot || (entries[*slot] && !sObjectMgr->GetItemTemplate(entries[*slot])) ||
                            !currentBot->CanRewardQuest(quest, false) || !currentBot->CanRewardQuest(quest, *slot, false)) continue;
                        WorldPackets::Quest::QuestGiverChooseReward packet(WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD, 0));
                        packet.QuestGiverGUID = giver; packet.QuestID = int32(id); packet.ItemChoiceID = *slot;
                        ++attempted;
                        session.HandleQuestgiverChooseRewardOpcode(packet);
                        if (session.GetPlayer() == currentBot && currentBot->GetQuestRewardStatus(id)) ++accepted;
                        continue;
                    }
                    if (!quest || quest->IsTurnIn() || !currentGiver->hasQuest(id) || currentBot->FindQuestSlot(id) < MAX_QUEST_LOG_SIZE) continue;
                    WorldPackets::Quest::QuestGiverAcceptQuest packet(WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST, 0));
                    packet.QuestGiverGUID = giver; packet.QuestID = id; packet.StartCheat = 0;
                    ++attempted;
                    session.HandleQuestgiverAcceptQuestOpcode(packet);
                    if (session.GetPlayer() == currentBot && currentBot->FindQuestSlot(id) < MAX_QUEST_LOG_SIZE) ++accepted;
                }
                bot = session.GetPlayer();
                requester = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
                if (requester && requester->GetSession() && !requester->GetSession()->IsServerOrigin())
                    ChatHandler(requester->GetSession()).PSendSysMessage("Playerbot %s %s *: %u native completions from %u attempts; %u candidates. Partial results are not rolled back.",
                        bot ? bot->GetName().c_str() : "unavailable", rewardAll ? "reward" : "accept", accepted, attempted, uint32(candidates.size()));
                outcome = "bounded native quest batch finished; see counts";
            }
            else if (reward)
            {
                Quest const* quest = sObjectMgr->GetQuestTemplate(request->Quest);
                if (quest && !quest->IsRepeatable() && !quest->IsTurnIn())
                {
                    std::array<uint32, 6> entries{}, counts{};
                    static_assert(QUEST_REWARD_CHOICES_COUNT == 6);
                    std::copy_n(quest->RewardChoiceItemId, 6, entries.begin());
                    std::copy_n(quest->RewardChoiceItemCount, 6, counts.begin());
                    auto slot = PlayerbotQuestReward::ResolveItem(entries, counts, request->Item);
                    if (bot->GetQuestRewardStatus(request->Quest)) outcome = "already rewarded";
                    else if (slot && bot->GetQuestStatus(request->Quest) == QUEST_STATUS_COMPLETE &&
                        (!request->Item || sObjectMgr->GetItemTemplate(request->Item)) &&
                        bot->CanRewardQuest(quest, false) && bot->CanRewardQuest(quest, *slot, false))
                    {
                        WorldPackets::Quest::QuestGiverChooseReward packet(WorldPacket(CMSG_QUEST_GIVER_CHOOSE_REWARD, 0));
                        packet.QuestGiverGUID = giver;
                        packet.QuestID = int32(request->Quest);
                        packet.ItemChoiceID = *slot;
                        session.HandleQuestgiverChooseRewardOpcode(packet);
                        outcome = bot->GetQuestRewardStatus(request->Quest) ? "reward confirmed" : "reward not confirmed";
                    }
                    else outcome = "reward rejected by native eligibility or choice checks";
                }
                else outcome = "reward requires an ordinary non-repeatable quest";
            }
            else
            {
                bool alreadyPresent = bot->FindQuestSlot(request->Quest) < MAX_QUEST_LOG_SIZE;
                if (!alreadyPresent)
                {
                    WorldPackets::Quest::QuestGiverAcceptQuest packet(WorldPacket(CMSG_QUEST_GIVER_ACCEPT_QUEST, 0));
                    packet.QuestGiverGUID = giver;
                    packet.QuestID = request->Quest;
                    packet.StartCheat = 0;
                    session.HandleQuestgiverAcceptQuestOpcode(packet);
                }
                outcome = alreadyPresent ? "already present" :
                    bot->FindQuestSlot(request->Quest) < MAX_QUEST_LOG_SIZE ? "accepted" : "not accepted by native checks";
            }
        }
    }
    mailbox.Finish(request->Serial);
    if (requester && requester->GetSession() && !requester->GetSession()->IsServerOrigin())
    {
        if (inspect && !inspectionReported)
            ChatHandler(requester->GetSession()).PSendSysMessage("Playerbot %s quest inspection: %s.", bot ? bot->GetName().c_str() : "unavailable", outcome);
        else if (!inspect)
            ChatHandler(requester->GetSession()).PSendSysMessage("Playerbot %s: quest %u %s.",
                bot ? bot->GetName().c_str() : "unavailable", request->Quest, outcome);
    }
    TC_LOG_INFO("module.playerbots", "PB-QUEST: %s NPC %s quest %u item %u: %s",
        bot ? bot->GetName().c_str() : "unavailable", abandon ? "abandon" : share ? "share" : inspect ? "inspect" : reward ? "reward" : "accept", request->Quest, request->Item, outcome);
}
