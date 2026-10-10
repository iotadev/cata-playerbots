/* Adapted from ItemUsageValue::IsItemUsefulForQuest at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_QUEST_ITEM_H
#define PLAYERBOT_QUEST_ITEM_H
#include "Define.h"
class Player;
struct ItemTemplate;
namespace PlayerbotQuestItem
{
inline bool Outstanding(uint32 objective, uint32 required, uint32 candidate, uint32 carried)
{
    return objective && candidate == objective && required && carried < required;
}
// Map-owner read only. A usefulness fact does not grant quest/loot permission.
bool Useful(Player const& player, ItemTemplate const& item);
// First connected master-sync slice: no reservation/transfer or inventory edit.
inline bool DeferCorpseQuestItem(bool enabled, bool questClass, bool humanNeedsItem, bool perPlayerCopy = false)
{
    return enabled && questClass && humanNeedsItem && !perPlayerCopy;
}
}
#endif
