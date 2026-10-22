/* Adapted from BuyAction at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. See PORTING.md for human-directed purchase scope. */
#include "PlayerbotPurchase.h"
#include "PlayerbotConsumableUsage.h"
#include "PlayerbotPotionStrategy.h"
#include "../../Script/PlayerbotConfig.h"
#include "PlayerbotSecurity.h"
#include "Chat.h"
#include "Creature.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "Timer.h"
#include "WorldSession.h"
#include <algorithm>
namespace
{
using namespace PlayerbotPurchase;
using PlayerbotNpcRequest::Request;
Category CategoryFor(ItemTemplate const& item, Player const& bot)
{
    using Type = PlayerbotConsumable::Type;
    switch (PlayerbotConsumable::ConsumableType(item, bot.GetMaxPower(POWER_MANA) > 0))
    {
        case Type::Food: return Category::Food;
        case Type::Drink: return Category::Drink;
        case Type::Bandage: return Category::Bandage;
        case Type::HealingPotion:
            return PlayerbotPotion::RecoverySpell(item, PlayerbotPotion::Kind::Healing) ? Category::HealingPotion : Category::Unsupported;
        case Type::ManaPotion:
            return PlayerbotPotion::RecoverySpell(item, PlayerbotPotion::Kind::Mana) ? Category::ManaPotion : Category::Unsupported;
        default: return Category::Unsupported;
    }
}
Creature* Ready(WorldSession& session, Request const& request, Player*& bot, Player*& human)
{
    bot = session.GetPlayer();
    human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request.Requester));
    bool attached = session.GetServerOriginFollowTargetGuidLow() == request.Requester ||
        session.GetServerOriginPartyControllerGuidLow() == request.Requester;
    bool eligible = PlayerbotModulePurchaseEnabled() && PlayerbotNpcRequest::Fresh(request, getMSTime()) &&
        session.IsServerOrigin() && !session.PlayerLoading() && !session.isLogingOut() && bot && bot->IsInWorld() &&
        bot->IsAlive() && !bot->IsBeingTeleported() && !bot->IsInCombat() && !bot->IsInFlight() && !bot->IsNonMeleeSpellCast(false) &&
        attached && human && human->GetSession() && !human->GetSession()->IsServerOrigin() && human->IsInWorld() &&
        !human->IsBeingTeleported() && bot->GetMapId() == request.Map && bot->GetInstanceId() == request.Instance &&
        human->GetMap() == bot->GetMap() && PlayerbotSecurity(*bot).CheckLevelFor(PLAYERBOT_SECURITY_ALLOW_ALL, *human);
    ObjectGuid npc(request.Npc);
    Creature* vendor = eligible && npc.IsCreature() ? bot->GetNPCIfCanInteractWith(npc, UNIT_NPC_FLAG_VENDOR) : nullptr;
    return vendor && human->GetNPCIfCanInteractWith(npc, UNIT_NPC_FLAG_VENDOR) ? vendor : nullptr;
}
enum class Result { Bought, Blocked, Stopped };
struct Outcome { Result State = Result::Blocked; uint32 Quantity = 0; uint64 Spent = 0; };
Outcome Buy(WorldSession& session, Request const& request, Offer offer, bool requireNeed)
{
    Player* bot = nullptr; Player* human = nullptr;
    Creature* vendor = Ready(session, request, bot, human);
    if (!vendor) return {Result::Stopped};
    ItemTemplate const* item = sObjectMgr->GetItemTemplate(offer.Item);
    VendorItemData const* stock = vendor->GetVendorItems();
    VendorItem const* row = stock ? stock->GetItem(offer.Slot) : nullptr;
    auto type = item ? CategoryFor(*item, *bot) : Category::Unsupported;
    uint32 before = bot->GetItemCount(offer.Item, false);
    if (!item || !row || row->item != offer.Item || row->Type != ITEM_VENDOR_TYPE_ITEM || row->ExtendedCost ||
        type == Category::Unsupported || type != offer.Kind ||
        bot->CanUseItem(item) != EQUIP_ERR_OK || !BundleFits(item->GetBuyCount(), item->GetMaxStackSize(), before) ||
        (requireNeed && PlayerbotConsumable::StockUsageFor(*bot, *item) != PlayerbotConsumable::Usage::Use)) return {};
    uint64 money = bot->GetMoney();
    // Cata owns stock/conditions, reputation and aura discounts, payment and storage.
    bot->BuyItemFromVendorSlot(ObjectGuid(request.Npc), offer.Slot, offer.Item, item->GetBuyCount(), NULL_BAG, NULL_SLOT);
    bot = session.GetPlayer();
    if (!bot) return {Result::Stopped};
    uint32 after = bot->GetItemCount(offer.Item, false);
    return {after > before ? Result::Bought : Result::Blocked, after > before ? after - before : 0,
        money >= bot->GetMoney() ? money - bot->GetMoney() : 0};
}
}
void PlayerbotPurchase::Update(WorldSession& session, Mailbox& mailbox)
{
    auto request = mailbox.Take();
    if (!request) return;
    Player* bot = nullptr; Player* human = nullptr;
    Creature* vendor = Ready(session, *request, bot, human);
    uint32 quantities[Categories] = {}, bundles = 0;
    uint64 spent = 0;
    bool stopped = !vendor;
    if (vendor)
    {
        VendorItemData const* stock = vendor->GetVendorItems();
        Choices choices;
        uint32 rows = stock ? stock->GetItemCount() : 0;
        if (!request->Item) rows = std::min(rows, MaxVendorRows);
        for (uint32 slot = 0; slot < rows; ++slot)
        {
            VendorItem const* row = stock->GetItem(slot);
            if (!row || row->Type != ITEM_VENDOR_TYPE_ITEM || row->ExtendedCost || (request->Item && row->item != request->Item)) continue;
            ItemTemplate const* item = sObjectMgr->GetItemTemplate(row->item);
            auto type = item ? CategoryFor(*item, *bot) : Category::Unsupported;
            if (!item || type == Category::Unsupported ||
                bot->CanUseItem(item) != EQUIP_ERR_OK || !BundleFits(item->GetBuyCount(), item->GetMaxStackSize(), bot->GetItemCount(row->item, false))) continue;
            Offer offer{row->item, slot, item->GetBaseItemLevel(), type};
            if (request->Item) { choices[uint32(type)] = offer; break; }
            if (PlayerbotConsumable::StockUsageFor(*bot, *item) == PlayerbotConsumable::Usage::Use)
                Consider(choices, offer);
        }
        // Only copied identities survive native calls. Re-resolve all facts per bundle.
        for (uint32 type = 0; type < choices.size() && !stopped; ++type)
        {
            if (!choices[type]) continue;
            uint32 limit = request->Item ? 1 : MaxPurchasesPerType;
            for (uint32 purchase = 0; purchase < limit; ++purchase)
            {
                Outcome result = Buy(session, *request, *choices[type], !request->Item);
                spent += result.Spent;
                quantities[type] += result.Quantity;
                stopped = result.State == Result::Stopped;
                if (result.State != Result::Bought) break;
                ++bundles;
            }
        }
    }
    human = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(request->Requester));
    bot = session.GetPlayer();
    std::string label = request->Item ? std::to_string(request->Item) : "vendor";
    if (human && human->GetSession())
        ChatHandler(human->GetSession()).PSendSysMessage(
            "Playerbot %s: buy %s finished; food +%u, drink +%u, healing potion +%u, mana potion +%u, bandage +%u, %u bundle(s), spent %llu copper. %s",
            bot ? bot->GetName().c_str() : "unavailable", label.c_str(),
            quantities[0], quantities[1], quantities[2], quantities[3], quantities[4], bundles, static_cast<unsigned long long>(spent),
            stopped ? "Stopped after state, control or location changed; partial purchases remain." :
            bundles ? "Native inventory increases observed; saved inventory not verified." : "No purchase observed; need, stock, funds or native eligibility may block it.");
    mailbox.Finish(request->Serial);
}
