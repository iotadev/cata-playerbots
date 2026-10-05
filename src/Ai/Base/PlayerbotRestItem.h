/* Cata adaptation of donor FindFoodVisitor at
 * 037c01418b5d01506917a3db9b44fd56ac5f965c. GPL v2 or later. See PORTING.md. */
#ifndef PLAYERBOT_REST_ITEM_H
#define PLAYERBOT_REST_ITEM_H
#include "SharedDefines.h"
#include "ItemTemplate.h"
#include <cstdint>
namespace PlayerbotRest
{
inline bool FoodItem(std::uint32_t itemClass, std::uint32_t subclass)
{
    return itemClass == ITEM_CLASS_CONSUMABLE &&
        (subclass == ITEM_SUBCLASS_CONSUMABLE || subclass == ITEM_SUBCLASS_FOOD);
}
inline std::uint32_t ItemCategory(std::uint32_t itemCategory, std::uint32_t spellCategory)
{
    return itemCategory ? itemCategory : spellCategory;
}
// Map-owned values only. Preserve the item-selected mode rather than infer it
// again from SpellInfo, whose category can differ from the on-use item effect.
struct ActiveRest
{
    std::uint32_t Spell = 0;
    bool Drinking = false;
    void Begin(std::uint32_t spell, bool drinking) { Spell = spell; Drinking = drinking; }
    void Clear() { Spell = 0; Drinking = false; }
    bool Finished(float health, float mana) const { return (Drinking ? mana : health) >= 95.0f; }
};
}
#endif
