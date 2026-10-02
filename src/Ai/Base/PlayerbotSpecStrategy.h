/*
 * Cata adaptation of mod-playerbots AiFactory::AddDefaultCombatStrategies and
 * PlayerbotAI::SelectiveResetStrategies at 7bae1b5c58c76a0aa20381155edc08096d1485b2.
 * See AUTHORS.md and PORTING.md. Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOT_SPEC_STRATEGY_H
#define PLAYERBOT_SPEC_STRATEGY_H
#include "../../Bot/Engine/Engine.h"
#include "Player.h"

namespace PlayerbotSpec
{
// Only strategies actually implemented in the Cata contexts. No Wrath talent
// point counting or fabricated spec-specific rotations.
inline char const* CombatStrategy(uint8 playerClass, uint32 primaryTree)
{
    switch (playerClass)
    {
        case CLASS_WARRIOR:
            switch (primaryTree)
            {
                case TALENT_TREE_WARRIOR_PROTECTION: return "tank";
                case TALENT_TREE_WARRIOR_ARMS: return "arms";
                case TALENT_TREE_WARRIOR_FURY: return "fury";
                default: return "warrior";
            }
        case CLASS_MAGE:
            switch (primaryTree)
            {
                case TALENT_TREE_MAGE_FROST: return "frost";
                case TALENT_TREE_MAGE_FIRE: return "fire";
                case TALENT_TREE_MAGE_ARCANE: return "arcane";
                default: return "mage";
            }
        case CLASS_PRIEST:
            return "heal"; // explicit fallback until donor Priest specs are ported
        default:
            return nullptr;
    }
}
inline bool Refresh(Engine& engine, uint8 playerClass, uint32 primaryTree)
{
    char const* desired = CombatStrategy(playerClass, primaryTree);
    if (!desired || engine.HasStrategy(desired))
        return false;
    // Context sibling exclusivity removes only the previous combat strategy.
    // The donor engine's Init clears obsolete queued work and rebuilds triggers.
    engine.AddStrategy(desired);
    return engine.HasStrategy(desired);
}
}
#endif
