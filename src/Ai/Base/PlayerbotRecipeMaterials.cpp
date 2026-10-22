/* GPL v2 or later. Donor/Cata catalog adaptations are in PORTING.md. */
#include "PlayerbotRecipeMaterials.h"
#include "PlayerbotProfessionItem.h"
#include "ObjectMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "DBCStructure.h"
#include "Log.h"
#include <memory>
namespace
{
std::shared_ptr<PlayerbotRecipeMaterials::Index const> catalog;
bool SkillCompatible(uint32 skill, uint32 spell)
{
    auto bounds = sSpellMgr->GetSkillLineAbilityMapBounds(spell);
    bool any = false, matching = false;
    for (auto row = bounds.first; row != bounds.second; ++row)
    {
        if (!row->second || !row->second->SkillLine) continue;
        any = true; matching |= row->second->SkillLine == skill;
    }
    return !any || matching; // Recipe-only spell is allowed; another skill is not.
}
void AddFacts(PlayerbotRecipeMaterials::Index& index, uint32 skill, SpellInfo const& spell)
{
    if (!SkillCompatible(skill, spell.Id)) return;
    for (int32 reagent : spell.Reagent)
        if (reagent > 0) index.Add(skill, uint32(reagent));
    for (auto const& effect : spell.Effects)
        if (effect.Effect == SPELL_EFFECT_CREATE_ITEM && effect.ItemType) index.Add(skill, effect.ItemType);
}
}
void PlayerbotRecipeMaterials::Rebuild()
{
    auto result = std::make_shared<Index>();
    uint32 examined = 0, recipes = 0;
    for (auto const& entry : *sObjectMgr->GetItemTemplateStore())
    {
        if (examined++ >= MaxTemplates) { result->Truncated = true; break; }
        ItemTemplate const& recipe = entry.second;
        if (recipe.GetClass() != ITEM_CLASS_RECIPE) continue;
        uint32 skill = PlayerbotProfessionItem::RecipeSkill(recipe.GetSubClass());
        if (!skill || (recipe.GetRequiredSkill() && recipe.GetRequiredSkill() != skill)) continue;
        ++recipes;
        for (ItemEffect const& effect : recipe.Effects)
        {
            SpellInfo const* spell = effect.SpellID ? sSpellMgr->GetSpellInfo(effect.SpellID) : nullptr;
            if (!spell) continue;
            AddFacts(*result, skill, *spell);
            // Donor fallback follows one trigger level, not arbitrary recursion.
            for (auto const& part : spell->Effects)
                if (SpellInfo const* triggered = part.TriggerSpell ? sSpellMgr->GetSpellInfo(part.TriggerSpell) : nullptr)
                    AddFacts(*result, skill, *triggered);
            if (result->Truncated) break;
        }
        if (result->Truncated) break;
    }
    TC_LOG_INFO("module.playerbots", "Recipe material retention catalog: %u recipes, %u copied facts%s",
        recipes, uint32(result->Size()), result->Truncated ? " (bounded/partial)" : "");
    std::shared_ptr<Index const> published = std::move(result);
    std::atomic_store(&catalog, std::move(published));
}
void PlayerbotRecipeMaterials::Clear() { std::atomic_store(&catalog, std::shared_ptr<Index const>{}); }
bool PlayerbotRecipeMaterials::Contains(uint32 skill, uint32 item)
{
    auto snapshot = std::atomic_load(&catalog);
    return snapshot && snapshot->Contains(skill, item);
}
