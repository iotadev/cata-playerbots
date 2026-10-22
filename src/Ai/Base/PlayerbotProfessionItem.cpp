/* GPL v2 or later. Donor/Cata differences are recorded in PORTING.md. */
#include "PlayerbotProfessionItem.h"
#include "PlayerbotRecipeMaterials.h"
#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "DBCStores.h"
#include "DBCStructure.h"
#include <set>
namespace
{
bool MatchesSpell(uint32 item, SpellInfo const& spell)
{
    for (int32 reagent : spell.Reagent)
        if (PlayerbotProfessionItem::MaterialMatch(item, reagent, 0, false)) return true;
    for (auto const& effect : spell.Effects)
        if (PlayerbotProfessionItem::MaterialMatch(item, 0, effect.ItemType,
            effect.Effect == SPELL_EFFECT_CREATE_ITEM)) return true;
    return false;
}
bool MaterialsUseful(Player const& player, ItemTemplate const& item)
{
    using namespace PlayerbotProfessionItem;
    if (!MaterialClass(item.GetClass())) return false;
    std::set<uint32> skills;
    for (uint32 held : RelevantSkills)
        if (player.HasSkill(held))
            for (uint32 associated : MaterialSkills(held))
                if (associated) skills.insert(associated);
    uint32 examined = 0;
    for (uint32 skill : skills)
    {
        if (PlayerbotRecipeMaterials::Contains(skill, item.GetId())) return true;
        auto const* abilities = sDBCManager.GetSkillLineAbilitiesBySkill(skill);
        if (!abilities) continue;
        for (SkillLineAbilityEntry const* ability : *abilities)
        {
            if (examined++ >= MaxMaterialAbilities) return false;
            if (!ability || ability->SkillLine != skill) continue;
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(ability->Spell);
            if (!spell) continue;
            if (MatchesSpell(item.GetId(), *spell)) return true;
            // Same bounded one-level trigger inspection as donor IsCraftedBySpell.
            for (auto const& effect : spell->Effects)
                if (SpellInfo const* triggered = effect.TriggerSpell ? sSpellMgr->GetSpellInfo(effect.TriggerSpell) : nullptr)
                    if (MatchesSpell(item.GetId(), *triggered)) return true;
        }
    }
    return false; // Unknown, not a qualified negative or vendor permission.
}
}
bool PlayerbotProfessionItem::Useful(Player const& player, ItemTemplate const& item)
{
    if (MaterialsUseful(player, item)) return true;
    for (uint32 skill : ToolSkills(item.GetId()))
        if (skill && player.HasSkill(skill)) return true;
    if (item.GetClass() != ITEM_CLASS_RECIPE) return false;
    uint32 skill = RecipeSkill(item.GetSubClass());
    if (!skill || !player.HasSkill(skill)) return false;
    bool known = true, found = false, unlearned = false;
    auto learning = [&](uint32 spell)
    {
        if (!spell || !sSpellMgr->GetSpellInfo(spell)) { known = false; return; }
        found = true;
        unlearned |= !player.HasSpell(spell);
    };
    // Native Player::CastItemUseSpell supplies Effects[1] as the base-point
    // target for the first effect's generic learn wrapper, not TriggerSpell.
    if (!item.Effects.empty() && GenericLearn(item.Effects[0].SpellID))
    {
        if (!sSpellMgr->GetSpellInfo(item.Effects[0].SpellID)) return false;
        learning(GenericRecipeTarget(item.Effects[0].SpellID, uint32(item.Effects.size()),
            item.Effects.size() >= 2 ? item.Effects[1].SpellID : 0));
        return RetainRecipe(true, known && found, unlearned);
    }
    for (ItemEffect const& effect : item.Effects)
    {
        if (!effect.SpellID) continue;
        if (effect.Trigger == ITEM_SPELLTRIGGER_LEARN_SPELL_ID)
            learning(effect.SpellID);
        else if (effect.Trigger == ITEM_SPELLTRIGGER_ON_USE)
        {
            SpellInfo const* spell = sSpellMgr->GetSpellInfo(effect.SpellID);
            if (!spell) { known = false; continue; }
            for (auto const& part : spell->Effects)
                if (part.Effect == SPELL_EFFECT_LEARN_SPELL) learning(part.TriggerSpell);
        }
    }
    return RetainRecipe(true, known && found, unlearned);
}
