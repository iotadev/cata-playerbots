/* GPL v2 or later. Native map-owner read-only bag capacity facts. */
#include "PlayerbotBagUsage.h"
#include "Bag.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "Opcodes.h"
#include "WorldPacket.h"
#include "WorldSession.h"
namespace
{
bool General(ItemTemplate const& item)
{
    return item.GetClass() == ITEM_CLASS_CONTAINER && item.GetSubClass() == ITEM_SUBCLASS_CONTAINER &&
        item.GetInventoryType() == INVTYPE_BAG && !item.GetBagFamily() &&
        item.GetContainerSlots() && item.GetContainerSlots() <= PlayerbotBagUsage::MaxCapacity;
}
}
bool PlayerbotBagUsage::Useful(Player const& player, ItemTemplate const& candidate)
{
    static_assert(INVENTORY_SLOT_BAG_END - INVENTORY_SLOT_BAG_START == 4);
    static_assert(MaxCapacity == MAX_BAG_SIZE);
    if (!player.IsInWorld() || player.IsBeingTeleported() || !General(candidate)) return false;
    std::array<Slot, 4> slots{};
    // Include all four Cata carried bag slots, unlike the donor's START+1 loop.
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        Item const* item = player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        auto& fact = slots[slot - INVENTORY_SLOT_BAG_START];
        if (!item) { fact = {true, true, 0}; continue; }
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || !General(*proto)) continue; // Never replace a specialized bag based on generic capacity.
        Bag const* bag = item->ToBag();
        if (!bag || bag->GetBagSize() != proto->GetContainerSlots()) continue;
        fact = {true, true, bag->GetBagSize()};
    }
    return CapacityUpgrade(candidate.GetContainerSlots(), player.CanUseItem(&candidate) == EQUIP_ERR_OK, slots);
}
namespace
{
bool Busy(Player const& owner, Item const& item)
{
    return item.IsInTrade() || owner.GetLootGUID() == item.GetGUID() ||
        item.HasFlag(ITEM_FIELD_FLAGS, ITEM_FIELD_FLAG_WRAPPED | ITEM_FIELD_FLAG_REFUNDABLE);
}
std::optional<std::vector<PlayerbotBagUsage::Content>> Contents(Player const& owner, Bag const& bag)
{
    using PlayerbotBagUsage::Content;
    if (bag.GetOwnerGUID() != owner.GetGUID() || bag.GetBagSize() > PlayerbotBagUsage::MaxCapacity) return {};
    std::vector<Content> result;
    for (uint32 slot = 0; slot < bag.GetBagSize(); ++slot)
        if (Item const* item = bag.GetItemByPos(slot))
        {
            if (!item->GetTemplate() || !item->GetCount() || item->GetOwnerGUID() != owner.GetGUID() ||
                item->IsBag() || Busy(owner, *item) ||
                item->GetBagSlot() != bag.GetSlot() || item->GetSlot() != slot) return {};
            Content fact{item->GetGUID().GetRawValue(), item->GetEntry(), item->GetCount(),
                item->GetItemRandomPropertyId(), item->GetItemSuffixFactor()};
            for (auto const& prior : result) if (prior.Guid == fact.Guid) return {};
            result.push_back(fact);
        }
    return result;
}
}
PlayerbotBagUsage::ApplyResult PlayerbotBagUsage::ApplyOne(WorldSession& session)
{
    Player* bot = session.GetPlayer();
    if (!session.IsServerOrigin() || session.PlayerLoading() || session.isLogingOut() ||
        !bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported() ||
        bot->IsInCombat() || bot->IsInFlight() || bot->IsNonMeleeSpellCast(false)) return ApplyResult::Rejected;
    std::array<Slot, 4> slots{};
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        auto& fact = slots[slot - INVENTORY_SLOT_BAG_START];
        if (!item) { fact = {true, true, 0}; continue; }
        ItemTemplate const* proto = item->GetTemplate();
        Bag const* bag = proto ? item->ToBag() : nullptr;
        if (proto && General(*proto) && bag && bag->GetBagSize() == proto->GetContainerSlots() &&
            item->GetOwnerGUID() == bot->GetGUID() && item->GetCount() == 1 && !Busy(*bot, *item) &&
            !item->GetItemRandomPropertyId() && !item->GetItemSuffixFactor() && Contents(*bot, *bag))
            fact = {true, true, bag->GetBagSize()};
    }
    Item* best = nullptr;
    uint32 capacity = 0;
    uint8 destinationSlot = NULL_SLOT;
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
    {
        Item* item = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
        Bag const* bag = item && proto ? item->ToBag() : nullptr;
        if (!item || !proto || !SafeCarriedBag(item->GetOwnerGUID() == bot->GetGUID(),
            item->GetBagSlot() == INVENTORY_SLOT_BAG_0, General(*proto), bag && bag->IsEmpty(),
            Busy(*bot, *item), item->GetCount(),
            item->GetItemRandomPropertyId() != 0 || item->GetItemSuffixFactor() != 0) ||
            !bag || bag->GetBagSize() != proto->GetContainerSlots()) continue;
        auto destination = Destination(proto->GetContainerSlots(), bot->CanUseItem(item) == EQUIP_ERR_OK, slots);
        if (destination && proto->GetContainerSlots() > capacity)
        {
            best = item; capacity = proto->GetContainerSlots();
            destinationSlot = INVENTORY_SLOT_BAG_START + *destination;
        }
    }
    if (!best) return ApplyResult::NoChange;
    ObjectGuid identity = best->GetGUID();
    uint16 source = best->GetPos(), destination = 0;
    Item* old = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, destinationSlot);
    ObjectGuid displaced = old ? old->GetGUID() : ObjectGuid::Empty;
    std::vector<Content> contents;
    if (old)
    {
        Bag const* bag = old->ToBag();
        auto snapshot = bag ? Contents(*bot, *bag) : std::nullopt;
        if (!snapshot) return ApplyResult::Rejected;
        contents = std::move(*snapshot);
    }
    if (bot->CanEquipItem(destinationSlot, destination, best, old != nullptr, true) != EQUIP_ERR_OK ||
        destination != uint16(uint16(INVENTORY_SLOT_BAG_0) << 8 | destinationSlot)) return ApplyResult::Rejected;
    WorldPacket packet(CMSG_AUTOEQUIP_ITEM_SLOT, 9);
    packet << identity << destinationSlot;
    session.HandleAutoEquipItemSlotOpcode(packet); // Native SwapItem validates and migrates contents.
    Item* equipped = bot->GetItemByPos(INVENTORY_SLOT_BAG_0, destinationSlot);
    Item* returned = bot->GetItemByPos(uint8(source >> 8), uint8(source));
    Bag const* equippedBag = equipped ? equipped->ToBag() : nullptr;
    auto actual = equippedBag ? Contents(*bot, *equippedBag) : std::nullopt;
    bool restored = displaced.IsEmpty() ? !returned :
        returned && returned->GetGUID() == displaced && returned->GetOwnerGUID() == bot->GetGUID() &&
        returned->GetCount() == 1 && returned->ToBag() && returned->ToBag()->IsEmpty();
    return equipped && equipped->GetGUID() == identity && equipped->GetOwnerGUID() == bot->GetGUID() &&
        equipped->GetCount() == 1 && equippedBag && equippedBag->GetBagSize() == capacity &&
        restored && actual && SameContents(contents, *actual) ? ApplyResult::Confirmed : ApplyResult::Rejected;
}
