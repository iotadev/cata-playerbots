/* GPL v2 or later. See PORTING.md. */
#include "PlayerbotDisenchantItem.h"
#include "Bag.h"
#include "Item.h"
#include "Player.h"
bool PlayerbotDisenchantItem::Useful(Player const& player, uint32 entry, int32 property,
    PlayerbotEquipment::Survey const& survey)
{
    if (!player.IsInWorld() || player.IsBeingTeleported() || !entry || !survey.Available ||
        survey.Owner != player.GetGUID()) return false;
    Item const* match = nullptr;
    bool ambiguous = false;
    auto consider = [&](Item const* item)
    {
        if (!item || item->GetEntry() != entry || item->GetItemRandomPropertyId() != property ||
            item->GetOwnerGUID() != player.GetGUID()) return;
        if (match) ambiguous = true;
        else match = item;
    };
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        consider(player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
        if (Bag const* bag = player.GetBagByPos(slot))
            for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot) consider(bag->GetItemByPos(slot));
    if (!match || ambiguous || !NonUpgrade(survey, match->GetGUID(), entry, property, match->GetItemSuffixFactor())) return false;
    ItemTemplate const* item = match->GetTemplate();
    return item && Eligible(item->GetClass(), item->GetQuality(), item->GetBonding(), match->IsSoulBound(),
        player.HasSkill(SKILL_ENCHANTING), player.GetSkillValue(SKILL_ENCHANTING),
        item->RequiredDisenchantSkill, item->DisenchantID);
}
