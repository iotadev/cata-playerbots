/* Adapted from donor ItemUsageValue::Calculate/GetConsumableType/CurrentStacks/
 * BetterStacks at 037c01418b5d01506917a3db9b44fd56ac5f965c.
 * GPL v2 or later. See AUTHORS.md and PORTING.md. */
#ifndef PLAYERBOT_CONSUMABLE_USAGE_H
#define PLAYERBOT_CONSUMABLE_USAGE_H
#include "../../Bot/Engine/AiObjectContext.h"
#include "PlayerbotRestItem.h"
#include <charconv>
#include <cmath>
#include <string_view>
namespace PlayerbotConsumable
{
enum class Type { None, Food, Drink, HealingPotion, ManaPotion, Bandage };
// Unsupported is not donor ITEM_USAGE_NONE: other item-usage branches are unported.
enum class Usage { Unsupported, None, Use, Keep };
inline uint32 ParseItem(std::string_view text)
{
    if (text.empty() || text.size() > 10) return 0;
    uint32 id = 0;
    auto result = std::from_chars(text.data(), text.data() + text.size(), id);
    return result.ec == std::errc{} && result.ptr == text.data() + text.size() ? id : 0;
}
inline Type BaseType(uint32 itemClass, uint32 subclass, uint32 category, bool hasMana)
{
    if (itemClass != ITEM_CLASS_CONSUMABLE) return Type::None;
    if (PlayerbotRest::FoodItem(itemClass, subclass))
    {
        if (category == SPELL_CATEGORY_FOOD) return Type::Food;
        if (category == SPELL_CATEGORY_DRINK && hasMana) return Type::Drink;
    }
    return subclass == ITEM_SUBCLASS_BANDAGE ? Type::Bandage : Type::None;
}
inline Usage StockUsage(bool supported, bool usable, bool atMaximum, float current, float better)
{
    if (!supported) return Usage::Unsupported;
    if (!usable || atMaximum || !std::isfinite(current) || !std::isfinite(better) || current < 0 || better < 0)
        return Usage::None;
    if (better >= 2) return Usage::None;
    float total = current + better;
    return total < 2 ? Usage::Use : (total < 3 ? Usage::Keep : Usage::None);
}
void AddContexts(SharedNamedObjectContextList<UntypedValue>& values);
}
#endif
