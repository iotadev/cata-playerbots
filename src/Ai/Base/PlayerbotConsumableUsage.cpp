/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotConsumableUsage.h"
#include "../../Bot/PlayerbotAI.h"
#include "Bag.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include <map>
namespace
{
using namespace PlayerbotConsumable;
Type ConsumableType(ItemTemplate const& item, bool hasMana)
{
    if (item.GetClass() != ITEM_CLASS_CONSUMABLE) return Type::None;
    uint32 subclass = item.GetSubClass();
    bool firstUse = true;
    for (ItemEffect const& effect : item.Effects)
    {
        if (!effect.SpellID || effect.Trigger != ITEM_SPELLTRIGGER_ON_USE) continue;
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(effect.SpellID);
        if (!spell) continue;
        if (firstUse)
        {
            Type base = BaseType(item.GetClass(), subclass,
                PlayerbotRest::ItemCategory(effect.Category, spell->GetCategory()), hasMana);
            if (base != Type::None) return base;
        }
        firstUse = false;
        if (subclass != ITEM_SUBCLASS_POTION && subclass != ITEM_SUBCLASS_FLASK) continue;
        // Preserve donor effect order; Cata energize may restore a non-mana power.
        for (auto const& spellEffect : spell->Effects)
        {
            if (hasMana && spellEffect.Effect == SPELL_EFFECT_ENERGIZE && spellEffect.MiscValue == POWER_MANA)
                return Type::ManaPotion;
            if (spellEffect.Effect == SPELL_EFFECT_HEAL) return Type::HealingPotion;
        }
    }
    return BaseType(item.GetClass(), subclass, 0, hasMana);
}
class ConsumableUsageValue final : public CalculatedValue<Usage>, public Qualified
{
public:
    explicit ConsumableUsageValue(PlayerbotAI* ai) : CalculatedValue(ai, "consumable usage") { }
protected:
    Usage Calculate() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        uint32 id = ParseItem(qualifier);
        ItemTemplate const* item = id ? sObjectMgr->GetItemTemplate(id) : nullptr;
        if (!bot || !bot->IsInWorld() || bot->IsBeingTeleported() || !item) return Usage::Unsupported;
        bool hasMana = bot->GetMaxPower(POWER_MANA) > 0;
        Type type = ConsumableType(*item, hasMana);
        if (type == Type::None) return Usage::Unsupported;
        // Count carried inventory only. Aggregate identities before evaluating
        // better stock, so split stacks of one item cannot multiply its total.
        std::map<uint32, uint64> counts;
        auto count = [&](Item* carried)
        {
            if (carried && carried->GetCount()) counts[carried->GetEntry()] += carried->GetCount();
        };
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            count(bot->GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            if (Bag* bag = bot->GetBagByPos(slot))
                for (uint32 bagSlot = 0; bagSlot < bag->GetBagSize(); ++bagSlot)
                    count(bag->GetItemByPos(bagSlot));
        float current = float(counts[id]) / item->GetMaxStackSize();
        float better = 0;
        for (auto const& [otherId, amount] : counts)
        {
            ItemTemplate const* other = otherId != id ? sObjectMgr->GetItemTemplate(otherId) : nullptr;
            if (other && other->GetClass() == item->GetClass() && other->GetSubClass() == item->GetSubClass() &&
                other->GetBaseItemLevel() >= item->GetBaseItemLevel() && ConsumableType(*other, hasMana) == type &&
                bot->CanUseItem(other) == EQUIP_ERR_OK)
                better += float(amount) / other->GetMaxStackSize();
        }
        return StockUsage(true, bot->CanUseItem(item) == EQUIP_ERR_OK,
            item->GetMaxCount() && counts[id] >= item->GetMaxCount(), current, better);
    }
};
}
void PlayerbotConsumable::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["consumable usage"] = [](PlayerbotAI* ai) { return new ConsumableUsageValue(ai); };
    values.Add(factory);
}
