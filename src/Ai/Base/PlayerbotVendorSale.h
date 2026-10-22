/* SellAction native sale and ItemUsageValue retention ordering at
 * 79bd428115c8f33b74b13de23b133d49c3f75c60. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_VENDOR_SALE_H
#define PLAYERBOT_VENDOR_SALE_H
#include "PlayerbotNpcRequest.h"
#include "ItemTemplate.h"
#include <string_view>
class WorldSession;
namespace PlayerbotVendorSale
{
using PlayerbotNpcRequest::Mailbox;
constexpr uint32 MaxSales = 8;
inline bool Command(std::string_view text) { return text == "sell vendor"; }
inline bool QuestReference(uint32 item, uint32 objective, uint32 source)
{ return item && (item == objective || item == source); }
struct Facts
{
    bool OwnedCarried = false, PlainJunk = false, NoProfessionNeeds = false, NoGuildNeeds = false;
    bool QuestFactsKnown = false, QuestNeeded = true, Busy = true;
    uint32 Count = 0, SellPrice = 0;
};
inline bool Allowed(Facts const& facts)
{
    return facts.OwnedCarried && facts.PlainJunk && facts.NoProfessionNeeds && facts.NoGuildNeeds &&
        facts.QuestFactsKnown && !facts.QuestNeeded && !facts.Busy && facts.Count && facts.SellPrice;
}
inline bool PlainTemplate(uint32 itemClass, uint32 subclass, uint32 quality, uint32 binding,
    uint32 startQuest, uint32 requiredSkill, uint32 totem, bool hasEffects, bool affixed)
{
    return itemClass == ITEM_CLASS_MISCELLANEOUS && subclass == ITEM_SUBCLASS_JUNK &&
        quality == ITEM_QUALITY_POOR && binding == BIND_NONE && !startQuest && !requiredSkill &&
        !totem && !hasEffects && !affixed;
}
void Update(WorldSession& session, Mailbox& mailbox);
}
#endif
