/* Cata adaptation of donor InventoryAction / FindFoodVisitor / FindPotionVisitor
 * at 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#include "PlayerbotReadyCheckSupplies.h"
#include "PlayerbotPotionStrategy.h"
#include "Bag.h"
#include "Item.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

PlayerbotReadyCheck::Supplies PlayerbotReadyCheck::CarriedSupplies(Player& bot)
{
    Supplies supplies;
    auto count = [&](Item* item)
    {
        if (!item || !item->GetCount() || bot.CanUseItem(item) != EQUIP_ERR_OK) return;
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto || proto->GetClass() != ITEM_CLASS_CONSUMABLE) return;
        bool foodItem = proto->GetSubClass() == ITEM_SUBCLASS_CONSUMABLE || proto->GetSubClass() == ITEM_SUBCLASS_FOOD;
        bool food = false, drink = false, firstUse = true;
        // Stock is not current cast readiness: cooldown/need checks belong to execution.
        bool heal = PlayerbotPotion::RecoverySpell(*item, PlayerbotPotion::Kind::Healing) != nullptr;
        bool mana = PlayerbotPotion::RecoverySpell(*item, PlayerbotPotion::Kind::Mana) != nullptr;
        for (ItemEffect const& effect : proto->Effects)
        {
            if (!effect.SpellID || effect.Trigger != ITEM_SPELLTRIGGER_ON_USE) continue;
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(effect.SpellID);
            if (!spell) continue;
            if (firstUse && foodItem)
            {
                uint32 category = effect.Category ? effect.Category : spell->GetCategory();
                food = category == SPELL_CATEGORY_FOOD;
                drink = category == SPELL_CATEGORY_DRINK;
            }
            firstUse = false;
        }
        CountItem(supplies, item->GetCount(), true, food, drink, heal, mana);
    };
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        count(bot.GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = bot.GetBagByPos(bagSlot))
            for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot)
                count(bag->GetItemByPos(slot));
    return supplies;
}
