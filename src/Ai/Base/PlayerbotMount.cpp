/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotMount.h"
#include "PlayerbotCombatMovement.h"
#include "PlayerbotCombatDecision.h"
#include "PlayerbotTargetSelection.h"
#include "../../Bot/PlayerbotAI.h"
#include "../../Script/PlayerbotConfig.h"
#include "DBCStores.h"
#include "MotionMaster.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellMgr.h"
#include <algorithm>
#include <vector>

namespace
{
bool GroundSpell(Player& bot, SpellInfo const& spell, int32& speed)
{
    bool mounted = false, ground = false, flying = false, underwater = false;
    bool capabilityChecked = false, capabilityGround = true;
    speed = 0;
    for (SpellEffectInfo const& effect : spell.Effects)
    {
        flying = flying || effect.ApplyAuraName == SPELL_AURA_FLY ||
            effect.ApplyAuraName == SPELL_AURA_MOD_INCREASE_MOUNTED_FLIGHT_SPEED;
        if (effect.ApplyAuraName == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
        { ground = true; speed = std::max(speed, effect.CalcValue(&bot)); }
        if (effect.ApplyAuraName != SPELL_AURA_MOUNTED) continue;
        mounted = true;
        if (!effect.MiscValueB) continue; // Legacy ground speed must establish eligibility below.
        MountCapabilityEntry const* capability = bot.GetMountCapability(uint32(effect.MiscValueB));
        if (!capability) return false;
        capabilityChecked = true;
        capabilityGround = capabilityGround && (capability->Flags & MOUNT_CAPABILITY_FLAG_GROUND) != 0;
        flying = flying || (capability->Flags & MOUNT_CAPABILITY_FLAG_FLYING) != 0;
        underwater = (capability->Flags & MOUNT_CAPABILITY_FLAG_UNDERWATER) != 0;
        if (SpellInfo const* modifier = sSpellMgr->GetSpellInfo(capability->ModSpellAuraID))
            for (SpellEffectInfo const& bonus : modifier->Effects)
                if (bonus.ApplyAuraName == SPELL_AURA_MOD_INCREASE_MOUNTED_SPEED)
                    speed = std::max(speed, bonus.CalcValue(&bot));
    }
    return PlayerbotMount::GroundCapability(mounted, capabilityChecked ? capabilityGround : ground, flying, underwater);
}
bool GroundMounted(Player& player)
{
    if (!player.IsMounted() || player.IsFlying() || player.IsInFlight() || player.GetVehicle()) return false;
    for (AuraEffect const* effect : player.GetAuraEffectsByType(SPELL_AURA_MOUNTED))
    {
        int32 speed = 0;
        if (GroundSpell(player, *effect->GetSpellInfo(), speed)) return true;
    }
    return false;
}
}
PlayerbotMount::Result PlayerbotMount::Update(PlayerbotAI& ai, State& state, bool following, bool command, uint32_t now)
{
    Player* bot = ai.GetBot();
    Player* owner = ai.GetController();
    if (!bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported() ||
        bot->IsInFlight() || bot->IsFlying() || bot->GetVehicle() ||
        bot->HasUnitMovementFlag(MOVEMENTFLAG_FALLING | MOVEMENTFLAG_FALLING_FAR))
        return {}; // Never perform flight/fall/vehicle cleanup as ground coordination.
    bool attached = following && owner && owner->IsInWorld() && owner->IsAlive() &&
        !owner->IsBeingTeleported() && owner->GetMap() == bot->GetMap();
    bool mountedOwner = attached && GroundMounted(*owner);
    bool busy = command || bot->IsInCombat() || bot->GetVictim() ||
        (attached && PlayerbotTargetSelection::HasNearbyPartyCombat(*bot, *owner)) ||
        ai.GetRestSpellId() || ai.LootRequests().Pending() || ai.LootPursuit().Active() || ai.Rebuff().IsPending(now);
    Spell* current = bot->GetCurrentSpell(CURRENT_GENERIC_SPELL);
    bool ownCast = state.Spell && current && current->GetSpellInfo()->Id == state.Spell;
    bool sameOwner = attached && state.Owner == uint64(owner->GetGUID());
    if (ShouldRelease(state.Spell != 0, PlayerbotModuleGroundMountEnabled(), attached, sameOwner, mountedOwner, busy) ||
        (ownCast && uint32_t(now - state.Started) >= 10000))
    {
        if (ownCast) bot->InterruptNonMeleeSpells(false, state.Spell);
        if (bot->HasAura(state.Spell)) bot->RemoveAurasDueToSpell(state.Spell);
        state.Forget();
        return {false, true};
    }
    if (state.Spell)
    {
        if (ownCast) return {true, false};
        if (bot->HasAura(state.Spell)) return {};
        if (uint32_t(now - state.Started) < 500) return {true, false};
        state.Forget();
        return {false, true}; // Failed/interrupted cast: allow native follow to resume.
    }
    bool allowed = attached && bot->IsOutdoors() && !bot->InBattleground() && !bot->InArena() &&
        !bot->IsInWater() && bot->GetShapeshiftForm() == FORM_NONE && !bot->IsInDisallowedMountForm() &&
        bot->getLevel() >= 20 && bot->GetSkillValue(SKILL_RIDING) >= 75 &&
        bot->IsWithinDistInMap(owner, 100.0f) && PlayerbotCombatMovement::CanMove(*bot) &&
        !bot->IsNonMeleeSpellCast(false);
    if (!ShouldMount(PlayerbotModuleGroundMountEnabled(), attached, mountedOwner, bot->IsMounted(), busy, allowed) ||
        !state.CanAttempt(now)) return {};
    state.Attempt(now);
    std::vector<Candidate> candidates;
    for (auto const& [id, learned] : bot->GetSpellMap())
    {
        if (learned.state == PLAYERSPELL_REMOVED || !learned.active || learned.disabled) continue;
        SpellInfo const* spell = sSpellMgr->GetSpellInfo(id);
        TriggerCastFlags flags = TRIGGERED_NONE;
        if (spell) spell = bot->GetCastSpellInfo(spell, flags);
        int32 speed = 0;
        if (!spell || spell->IsPassive() || !GroundSpell(*bot, *spell, speed)) continue;
        candidates.push_back({id, spell->Id, speed});
    }
    if (candidates.empty()) return {};
    std::sort(candidates.begin(), candidates.end(), Better);
    bot->GetMotionMaster()->Clear(MOTION_SLOT_ACTIVE);
    bot->GetMotionMaster()->MoveIdle();
    bot->StopMoving();
    for (Candidate const& candidate : candidates)
        if (PlayerbotDecision::TryCast(*bot, *bot, candidate.Base, "ground mount"))
        {
            state.Spell = candidate.Cast; state.Owner = uint64(owner->GetGUID()); state.Started = now;
            return {!bot->HasAura(candidate.Cast), true};
        }
    return {false, true}; // Native rejection of every candidate must not leave follow idle.
}
