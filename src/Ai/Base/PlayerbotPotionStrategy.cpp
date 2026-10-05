/* Cata native-item adaptation of donor UsePotionsStrategy / UseItemAction at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#include "PlayerbotPotionStrategy.h"
#include "../../Bot/PlayerbotAI.h"
#include "../../Script/PlayerbotConfig.h"
#include "Bag.h"
#include "Item.h"
#include "Log.h"
#include "Player.h"
#include "SpellCastRequest.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

namespace
{
using PlayerbotPotion::Kind;
bool Ready(PlayerbotAI* ai, Kind kind)
{
    Player* bot = ai ? ai->GetBot() : nullptr;
    Player* owner = ai ? ai->GetController() : nullptr;
    if (!bot || !owner || !bot->IsInWorld() || !owner->IsInWorld() || !owner->IsAlive() ||
        owner->IsBeingTeleported() || bot->GetMap() != owner->GetMap() ||
        !bot->IsWithinDistInMap(owner, 35.0f) || ai->GetRestSpellId() ||
        ai->LootRequests().Pending() || ai->LootPursuit().Active()) return false;
    bool unavailable = bot->IsMounted() || bot->IsInFlight() || bot->GetVehicle() || bot->IsCharming() ||
        bot->IsCharmed() || bot->InArena() ||
        bot->HasUnitState(UNIT_STATE_STUNNED | UNIT_STATE_CONFUSED | UNIT_STATE_FLEEING | UNIT_STATE_ISOLATED);
    if (!PlayerbotPotion::CanUse(PlayerbotModulePotionsEnabled(), bot->IsAlive(), bot->IsInCombat(),
        bot->IsBeingTeleported(), bot->IsNonMeleeSpellCast(false) || bot->HasUnitState(UNIT_STATE_CASTING),
        unavailable, PlayerbotPotion::PotionLockoutApplies(kind, bot->GetLastPotionId() != 0))) return false;
    int32 maximum = bot->GetMaxPower(POWER_MANA);
    float mana = maximum > 0 ? 100.0f * bot->GetPower(POWER_MANA) / maximum : 100.0f;
    return PlayerbotPotion::Needs(kind, bot->GetHealthPct(), mana, bot->GetPowerType() == POWER_MANA && maximum > 0);
}
}
SpellInfo const* PlayerbotPotion::RecoverySpell(Item const& item, Kind kind)
{
    ItemTemplate const* proto = item.GetTemplate();
    if (!proto || proto->GetClass() != ITEM_CLASS_CONSUMABLE) return nullptr;
    for (ItemEffect const& effect : proto->Effects)
        if (effect.SpellID && effect.Trigger == ITEM_SPELLTRIGGER_ON_USE)
        {
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(effect.SpellID);
            if (!spell) continue;
            if (!MatchesItem(true, proto->GetSubClass() == ITEM_SUBCLASS_POTION, spell->Id, kind)) return nullptr;
            if (!spell->CanBeUsedInCombat() || !spell->IsPositive() || spell->IsChanneled() || spell->CalcCastTime() != 0)
                return nullptr;
            if (kind != Kind::Mana) return spell->HasEffect(SPELL_EFFECT_HEAL) ? spell : nullptr;
            for (SpellEffectInfo const& entry : spell->Effects)
                if (entry.Effect == SPELL_EFFECT_ENERGIZE && entry.MiscValue == POWER_MANA) return spell;
            return nullptr; // Match the native first valid on-use effect, not a later spell.
        }
    return nullptr;
}
namespace
{
bool Usable(Player& bot, Item* item, Kind kind)
{
    SpellInfo const* spell = item && item->GetCount() ? PlayerbotPotion::RecoverySpell(*item, kind) : nullptr;
    return spell && bot.CanUseItem(item) == EQUIP_ERR_OK &&
        !bot.GetSpellHistory()->HasCooldown(spell, item->GetEntry()) &&
        bot.GetSpellHistory()->GetRemainingGlobalCooldown(spell) == 0 && bot.CanRequestSpellCast(spell);
}
ObjectGuid FindPotion(Player& bot, Kind kind)
{
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (Item* item = bot.GetItemByPos(INVENTORY_SLOT_BAG_0, slot); Usable(bot, item, kind)) return item->GetGUID();
    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = bot.GetBagByPos(bagSlot))
            for (uint32 slot = 0; slot < bag->GetBagSize(); ++slot)
                if (Item* item = bag->GetItemByPos(slot); Usable(bot, item, kind)) return item->GetGUID();
    return ObjectGuid::Empty;
}
class PotionTrigger final : public Trigger
{
public:
    PotionTrigger(PlayerbotAI* ai, char const* name, Kind kind) : Trigger(ai, name, 2), kind(kind) { }
    bool IsActive() override { return Ready(botAI, kind); }
private:
    Kind kind;
};
class PotionAction final : public Action
{
public:
    PotionAction(PlayerbotAI* ai, char const* name, Kind kind) : Action(ai, name), kind(kind) { }
    bool isUseful() override { return Ready(botAI, kind); }
    bool isPossible() override { return Ready(botAI, kind) && !FindPotion(*botAI->GetBot(), kind).IsEmpty(); }
    bool Execute([[maybe_unused]] Event event) override
    {
        if (!Ready(botAI, kind)) return false;
        Player* bot = botAI->GetBot();
        Item* item = bot->GetItemByGuid(FindPotion(*bot, kind));
        if (!Usable(*bot, item, kind) || !Player::IsInventoryPos(item->GetBagSlot(), item->GetSlot()) ||
            bot->GetUseableItemByPos(item->GetBagSlot(), item->GetSlot()) != item) return false;
        SpellInfo const* spell = PlayerbotPotion::RecoverySpell(*item, kind);
        uint32 entry = item->GetEntry();
        uint32 spellId = spell->Id;
        WorldPackets::Spells::SpellCastRequest request;
        request.SpellID = spellId;
        request.Target.Flags = TARGET_FLAG_UNIT;
        request.Target.Unit = bot->GetGUID();
        auto pending = std::make_unique<PendingSpellCastRequest>(std::move(request),
            SpellCastRequestItemData(item->GetBagSlot(), item->GetSlot(), item->GetGUID()));
        bot->RequestSpellCast(std::move(pending), spell);
        // Native execution may consume/delete the item; never dereference it after submission.
        TC_LOG_INFO("module.playerbots", "PB-POTION: %s submitted %s item %u spell %u",
            bot->GetName().c_str(), name.c_str(), entry, spellId);
        return true; // Submitted, not a claim of restoration or native cast success.
    }
private:
    Kind kind;
};
}
void PlayerbotPotion::AddContexts(SharedNamedObjectContextList<Strategy>& strategies,
    SharedNamedObjectContextList<Action>& actions, SharedNamedObjectContextList<Trigger>& triggers)
{
    auto* strategy = new NamedObjectContext<Strategy>();
    strategy->creators["potions"] = [](PlayerbotAI* ai) { return new PotionStrategy(ai); };
    strategies.Add(strategy);
    auto* action = new NamedObjectContext<Action>();
    action->creators["healthstone"] = [](PlayerbotAI* ai) { return new PotionAction(ai, "healthstone", Kind::Healthstone); };
    action->creators["healing potion"] = [](PlayerbotAI* ai) { return new PotionAction(ai, "healing potion", Kind::Healing); };
    action->creators["mana potion"] = [](PlayerbotAI* ai) { return new PotionAction(ai, "mana potion", Kind::Mana); };
    actions.Add(action);
    auto* trigger = new NamedObjectContext<Trigger>();
    trigger->creators["potion critical health"] = [](PlayerbotAI* ai) { return new PotionTrigger(ai, "potion critical health", Kind::Healing); };
    trigger->creators["potion medium mana"] = [](PlayerbotAI* ai) { return new PotionTrigger(ai, "potion medium mana", Kind::Mana); };
    triggers.Add(trigger);
}
