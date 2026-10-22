/* ItemUsageValue skill retention at 79bd428115c8f33b74b13de23b133d49c3f75c60.
 * GPL v2 or later. See PORTING.md for positive-only Cata scope. */
#ifndef PLAYERBOT_PROFESSION_ITEM_H
#define PLAYERBOT_PROFESSION_ITEM_H
#include "ItemTemplate.h"
#include "SharedDefines.h"
#include <array>
class Player;
namespace PlayerbotProfessionItem
{
constexpr uint32 MaxMaterialAbilities = 4096;
inline constexpr std::array<uint32, 15> RelevantSkills = {SKILL_TAILORING, SKILL_LEATHERWORKING, SKILL_ENGINEERING,
    SKILL_BLACKSMITHING, SKILL_ALCHEMY, SKILL_ENCHANTING, SKILL_FISHING, SKILL_FIRST_AID, SKILL_COOKING,
    SKILL_JEWELCRAFTING, SKILL_MINING, SKILL_SKINNING, SKILL_HERBALISM, SKILL_INSCRIPTION, SKILL_ARCHAEOLOGY};
inline bool MaterialClass(uint32 itemClass)
{
    return itemClass == ITEM_CLASS_TRADE_GOODS || itemClass == ITEM_CLASS_MISCELLANEOUS ||
        itemClass == ITEM_CLASS_REAGENT || itemClass == ITEM_CLASS_GEM;
}
inline std::array<uint32, 4> MaterialSkills(uint32 skill)
{
    switch (skill)
    {
        case SKILL_MINING: return {SKILL_MINING, SKILL_BLACKSMITHING, SKILL_JEWELCRAFTING, SKILL_ENGINEERING};
        case SKILL_SKINNING: return {SKILL_SKINNING, SKILL_LEATHERWORKING};
        case SKILL_HERBALISM: return {SKILL_HERBALISM, SKILL_ALCHEMY};
        case SKILL_TAILORING: case SKILL_LEATHERWORKING: case SKILL_ENGINEERING:
        case SKILL_BLACKSMITHING: case SKILL_ALCHEMY: case SKILL_ENCHANTING:
        case SKILL_FISHING: case SKILL_FIRST_AID: case SKILL_COOKING: case SKILL_JEWELCRAFTING:
        case SKILL_INSCRIPTION: case SKILL_ARCHAEOLOGY: return {skill};
        default: return {};
    }
}
inline bool MaterialMatch(uint32 item, int32 reagent, uint32 createdItem, bool createsItem)
{
    return item && ((reagent > 0 && item == uint32(reagent)) || (createsItem && item == createdItem));
}
inline std::array<uint32, 5> ToolSkills(uint32 item)
{
    switch (item)
    {
        case 756: case 778: case 1819: case 1893: case 1959: case 2901: case 9465: case 20723:
            return {SKILL_MINING};
        case 40772: return {SKILL_MINING, SKILL_ENGINEERING, SKILL_BLACKSMITHING, SKILL_COOKING, SKILL_SKINNING};
        case 40892: return {SKILL_MINING, SKILL_BLACKSMITHING};
        case 40893: return {SKILL_MINING, SKILL_SKINNING};
        case 5956: return {SKILL_BLACKSMITHING, SKILL_ENGINEERING};
        case 6219: return {SKILL_ENGINEERING};
        case 6218: case 6339: case 11130: case 11145: case 16207: return {SKILL_ENCHANTING};
        case 7005: case 12709: case 19901: return {SKILL_SKINNING};
        case 4471: case 4470: return {SKILL_COOKING};
        case 6256: return {SKILL_FISHING};
        default: return {};
    }
}
inline uint32 RecipeSkill(uint32 subclass)
{
    switch (subclass)
    {
        case ITEM_SUBCLASS_LEATHERWORKING_PATTERN: return SKILL_LEATHERWORKING;
        case ITEM_SUBCLASS_TAILORING_PATTERN: return SKILL_TAILORING;
        case ITEM_SUBCLASS_ENGINEERING_SCHEMATIC: return SKILL_ENGINEERING;
        case ITEM_SUBCLASS_BLACKSMITHING: return SKILL_BLACKSMITHING;
        case ITEM_SUBCLASS_COOKING_RECIPE: return SKILL_COOKING;
        case ITEM_SUBCLASS_ALCHEMY_RECIPE: return SKILL_ALCHEMY;
        case ITEM_SUBCLASS_FIRST_AID_MANUAL: return SKILL_FIRST_AID;
        case ITEM_SUBCLASS_ENCHANTING_FORMULA: return SKILL_ENCHANTING;
        case ITEM_SUBCLASS_FISHING_MANUAL: return SKILL_FISHING;
        // Native Cata recipe subclasses, absent from the donor's recipe switch.
        case ITEM_SUBCLASS_JEWELCRAFTING_RECIPE: return SKILL_JEWELCRAFTING;
        case ITEM_SUBCLASS_INSCRIPTION_TECHNIQUE: return SKILL_INSCRIPTION;
        default: return 0;
    }
}
inline bool RetainRecipe(bool hasSkill, bool learningFactsKnown, bool hasUnlearnedSpell)
{ return hasSkill && learningFactsKnown && hasUnlearnedSpell; }
inline bool GenericLearn(uint32 spell) { return spell == 483 || spell == 55884; }
inline uint32 GenericRecipeTarget(uint32 wrapper, uint32 effectCount, uint32 secondSpell)
{ return GenericLearn(wrapper) && effectCount >= 2 ? secondSpell : 0; }
// Positive retention only. False is unresolved, never permission to sell.
bool Useful(Player const& player, ItemTemplate const& item);
}
#endif
