/* GPL v2 or later. Fresh world-owner native sale; see PORTING.md. */
#include "PlayerbotVendorSale.h"
#include "PlayerbotProfessionItem.h"
#include "../../Script/PlayerbotConfig.h"
#include "PlayerbotSecurity.h"
#include "Bag.h"
#include "Chat.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "QuestDef.h"
#include "Timer.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <algorithm>
#include <vector>
namespace
{
using PlayerbotNpcRequest::Request;
bool Ready(WorldSession& session, Request const& request, Player*& bot, Player*& human)
{
    bot = session.GetPlayer();
    human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request.Requester));
    bool attached = session.GetServerOriginFollowTargetGuidLow() == request.Requester ||
        session.GetServerOriginPartyControllerGuidLow() == request.Requester;
    ObjectGuid npc(request.Npc);
    return !request.Item && PlayerbotModuleSellEnabled() && PlayerbotNpcRequest::Fresh(request, getMSTime()) &&
        session.IsServerOrigin() && !session.PlayerLoading() && !session.isLogingOut() && bot && bot->IsInWorld() &&
        bot->IsAlive() && !bot->IsBeingTeleported() && !bot->IsInCombat() && !bot->IsInFlight() && !bot->IsNonMeleeSpellCast(false) &&
        attached && human && human->GetSession() && !human->GetSession()->IsServerOrigin() && human->IsInWorld() &&
        !human->IsBeingTeleported() && bot->GetMapId() == request.Map && bot->GetInstanceId() == request.Instance &&
        human->GetMap() == bot->GetMap() && PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *human) &&
        npc.IsCreature() && bot->GetNPCIfCanInteractWith(npc, UNIT_NPC_FLAG_VENDOR) && human->GetNPCIfCanInteractWith(npc, UNIT_NPC_FLAG_VENDOR);
}
bool QuestMetadataKnown(Player const& player)
{
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
        if (uint32 id = player.GetQuestSlotQuestId(slot))
            if (!sObjectMgr->GetQuestTemplate(id)) return false;
    return true;
}
bool QuestReferenced(Player const& player, uint32 item)
{
    if (player.HasQuestForItem(item)) return true;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 id = player.GetQuestSlotQuestId(slot);
        if (!id) continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(id);
        if (!quest) return true;
        if (PlayerbotVendorSale::QuestReference(item, 0, quest->GetSrcItemId())) return true;
        for (uint32 objective : quest->RequiredItemId)
            if (PlayerbotVendorSale::QuestReference(item, objective, 0)) return true;
    }
    return false;
}
bool NoProfessionNeeds(Player const& player)
{
    return std::none_of(PlayerbotProfessionItem::RelevantSkills.begin(), PlayerbotProfessionItem::RelevantSkills.end(),
        [&](uint32 skill) { return player.HasSkill(skill); });
}
bool Carried(Item const& item)
{
    return item.GetBagSlot() == INVENTORY_SLOT_BAG_0 ?
        item.GetSlot() >= INVENTORY_SLOT_ITEM_START && item.GetSlot() < INVENTORY_SLOT_ITEM_END :
        item.GetBagSlot() >= INVENTORY_SLOT_BAG_START && item.GetBagSlot() < INVENTORY_SLOT_BAG_END;
}
bool Qualified(Player const& bot, Player const& human, Item const& item)
{
    using namespace PlayerbotVendorSale;
    ItemTemplate const* proto = item.GetTemplate();
    if (!proto) return false;
    if (proto->GetFlags() || proto->GetFlags2() || proto->GetHolidayID() || proto->GetDuration() ||
        proto->ScriptId || item.IsSoulBound()) return false;
    Facts facts;
    facts.OwnedCarried = item.GetOwnerGUID() == bot.GetGUID() && Carried(item);
    facts.PlainJunk = PlainTemplate(proto->GetClass(), proto->GetSubClass(), proto->GetQuality(), proto->GetBonding(),
        proto->GetStartQuest(), proto->GetRequiredSkill(), proto->GetTotemCategory(),
        std::any_of(proto->Effects.begin(), proto->Effects.end(), [](ItemEffect const& effect) { return effect.SpellID != 0; }),
        proto->GetRandomProperty() || proto->GetRandomSuffix() || item.GetItemRandomPropertyId() || item.GetItemSuffixFactor());
    if (!facts.OwnedCarried || !facts.PlainJunk) return false;
    facts.NoProfessionNeeds = NoProfessionNeeds(bot) && NoProfessionNeeds(human);
    facts.NoGuildNeeds = !bot.GetGuildId() && !human.GetGuildId();
    facts.QuestFactsKnown = QuestMetadataKnown(bot) && QuestMetadataKnown(human);
    // Keep complete quest stacks too: having enough now is not permission to
    // remove the very items needed for turn-in. No use effects are allowed here.
    facts.QuestNeeded = !facts.QuestFactsKnown || QuestReferenced(bot, item.GetEntry()) || QuestReferenced(human, item.GetEntry());
    facts.Busy = item.IsInTrade() || bot.GetLootGUID() == item.GetGUID() ||
        item.HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED | ITEM_FIELD_FLAG_REFUNDABLE);
    facts.Count = item.GetCount(); facts.SellPrice = proto->GetSellPrice();
    return Allowed(facts);
}
struct Candidate { ObjectGuid Guid; uint32 Entry, Count; };
}
void PlayerbotVendorSale::Update(WorldSession& session, Mailbox& mailbox)
{
    auto request = mailbox.Take();
    if (!request) return;
    Player* bot = nullptr; Player* human = nullptr;
    std::vector<Candidate> candidates;
    bool stopped = !Ready(session, *request, bot, human);
    if (!stopped)
    {
        auto consider = [&](Item const* item)
        {
            if (item && candidates.size() < MaxSales && Qualified(*bot, *human, *item))
                candidates.push_back({item->GetGUID(), item->GetEntry(), item->GetCount()});
        };
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            consider(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (Bag const* bag = bot->GetBagByPos(slot))
                for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot) consider(bag->GetItemByPos(slot));
    }
    uint32 sold = 0, retained = 0; uint64 earned = 0;
    for (Candidate const& candidate : candidates)
    {
        if (!Ready(session, *request, bot, human)) { stopped = true; break; }
        Item const* item = bot->GetItemByGuid(candidate.Guid);
        if (!item || item->GetEntry() != candidate.Entry || item->GetCount() != candidate.Count || !Qualified(*bot, *human, *item))
        { ++retained; continue; }
        uint32 before = bot->GetItemCount(candidate.Entry, false);
        uint64 money = bot->GetMoney();
        WorldPacket packet(CMSG_SELL_ITEM, 20);
        packet << ObjectGuid(request->Npc) << candidate.Guid << candidate.Count;
        session.HandleSellItemOpcode(packet);
        bot = session.GetPlayer();
        if (!bot) { stopped = true; break; }
        uint32 after = bot->GetItemCount(candidate.Entry, false);
        if (before >= after && before - after == candidate.Count) ++sold;
        else ++retained;
        if (bot->GetMoney() >= money) earned += bot->GetMoney() - money;
    }
    human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
    bot = session.GetPlayer();
    if (human && human->GetSession())
        ChatHandler(human->GetSession()).PSendSysMessage(
            "Playerbot %s: qualified-junk sale, %u stack(s) removed, %u selected stack(s) skipped/unconfirmed, %llu copper gained. %s",
            bot ? bot->GetName().c_str() : "unavailable", sold, retained, static_cast<unsigned long long>(earned),
            stopped ? "Stopped: request expired or control/state/location/vendor eligibility changed; partial sales remain." :
            "Only plain gray junk with complete no-need facts is eligible; other items remain protected. Native buyback is finite.");
    mailbox.Finish(request->Serial);
}
