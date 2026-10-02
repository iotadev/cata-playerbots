/* Adapted from donor EstimatedLifetimeValue / EstimatedGroupDpsValue at
 * 7bae1b5c58c76a0aa20381155edc08096d1485b2. See PORTING.md.
 * Released under GNU GPL v2 or any later version. */
#include "PlayerbotCombatValues.h"
#include "PlayerbotDpsEstimate.h"
#include "PlayerbotThreatStrategy.h"
#include "PlayerbotCombatBalance.h"
#include "PlayerbotTargetSelection.h"
#include "../../Bot/PlayerbotAI.h"
#include "Bag.h"
#include "Creature.h"
#include "Group.h"
#include "Item.h"
#include "MotionMaster.h"
#include "ObjectAccessor.h"
#include "ThreatManager.h"
#include "WorldSession.h"
#include <set>

namespace
{
bool IsImplementedTank(Player const& player)
{
    return player.getClass() == CLASS_WARRIOR &&
        player.GetPrimaryTalentTree(player.GetActiveSpec()) == TALENT_TREE_WARRIOR_PROTECTION;
}
uint32 MixedGearScore(Player& player)
{
    PlayerbotDpsEstimate::GearScores scores;
    auto consider = [&](Item* item)
    {
        if (!item) return;
        ItemTemplate const* info = item->GetTemplate();
        if (info && player.CanUseItem(info) == EQUIP_ERR_OK)
            scores.Add(info->GetInventoryType(), info->GetBaseItemLevel(), info->GetQuality());
    };
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        consider(player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        consider(player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot));
    for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
        if (Item* item = player.GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (item->IsBag())
            {
                Bag* bag = static_cast<Bag*>(item);
                for (uint32 index = 0; index < bag->GetBagSize(); ++index)
                    consider(bag->GetItemByPos(index));
            }
    return scores.TopTwelve();
}
class EstimatedGroupDpsValue final : public CalculatedValue<float>
{
public:
    explicit EstimatedGroupDpsValue(PlayerbotAI* ai) : CalculatedValue(ai, "estimated group dps", 20000) { }
    float Get() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsAlive() || !PlayerbotDpsEstimate::BasicDps(bot->getLevel())) return 0.0f;
        ObjectGuid group = bot->GetGroup() ? bot->GetGroup()->GetGUID() : ObjectGuid::Empty;
        if (group != lastGroup || bot->GetMapId() != lastMap || bot->getLevel() != lastLevel)
        {
            Reset();
            lastGroup = group;
            lastMap = bot->GetMapId();
            lastLevel = bot->getLevel();
        }
        return CalculatedValue<float>::Get();
    }
protected:
    float Calculate() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsAlive()) return 0.0f;
        std::vector<Player*> members = {bot};
        if (Group* group = bot->GetGroup())
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->GetSource();
                if (member && member != bot && member->IsInWorld() && member->IsAlive() &&
                    member->GetSession() && member->GetSession()->IsServerOrigin() &&
                    member->GetMap() == bot->GetMap() && bot->GetExactDist(member) <= 100.0f)
                    members.push_back(member);
            }
        float dps = 0.0f;
        for (Player* member : members)
        {
            // The role follows implemented strategies, not unported Shadow/talent roles.
            if (!PlayerbotDpsEstimate::BasicDps(member->getLevel()) ||
                (member->getClass() != CLASS_WARRIOR && member->getClass() != CLASS_MAGE && member->getClass() != CLASS_PRIEST))
                return 0.0f;
            dps += PlayerbotDpsEstimate::Contribution(member->getLevel(), MixedGearScore(*member),
                IsImplementedTank(*member), member->getClass() == CLASS_PRIEST);
        }
        return dps * PlayerbotDpsEstimate::GroupBonus(members.size());
    }
private:
    ObjectGuid lastGroup;
    uint32 lastMap = 0;
    uint32 lastLevel = 0;
};
class EstimatedLifetimeValue final : public CalculatedValue<float>, public Qualified
{
public:
    explicit EstimatedLifetimeValue(PlayerbotAI* ai) : CalculatedValue(ai, "estimated lifetime") { }
protected:
    float Calculate() override
    {
        if (!botAI || (!qualifier.empty() && qualifier != "current target")) return 0.0f;
        Creature* target = botAI->GetCurrentTarget();
        AiObjectContext* context = botAI->GetAiObjectContext();
        Value<float>* dps = context ? context->GetValue<float>("estimated group dps") : nullptr;
        return target && target->IsAlive() && dps ?
            PlayerbotDpsEstimate::Lifetime(float(target->GetHealth()), dps->Get()) : 0.0f;
    }
};
// Read native PvE threat references only; these values never select an attack target.
bool ValidEngagedCreature(Player& bot, Player* controller, Creature& target)
{
    if (!target.IsInWorld() || !target.IsAlive() || target.GetMap() != bot.GetMap() ||
        target.IsControlledByPlayer() || target.IsInEvadeMode() || target.IsPolymorphed() ||
        !bot.IsValidAttackTarget(&target) || !bot.CanSeeOrDetect(&target) ||
        bot.GetExactDist2d(&target) >= 100.0f || !bot.IsWithinLOSInMap(&target) ||
        (target.IsImmunedToDamage(SPELL_SCHOOL_MASK_NORMAL) && target.IsImmunedToDamage(SPELL_SCHOOL_MASK_MAGIC)))
        return false;
    if (target.isTappedBy(&bot) || target.IsInCombatWith(&bot) ||
        (controller && controller->GetMap() == bot.GetMap() && target.GetThreatManager().GetThreat(controller) > 0))
        return true;
    if (target.hasLootRecipient()) return false;
    Unit* victim = target.GetVictim();
    Player* victimOwner = victim ? victim->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    return !victimOwner || victimOwner == &bot || victimOwner == controller ||
        (bot.GetGroup() && bot.GetGroup() == victimOwner->GetGroup());
}
class AttackersValue final : public CalculatedValue<std::vector<ObjectGuid>>
{
public:
    explicit AttackersValue(PlayerbotAI* ai) : CalculatedValue(ai, "attackers") { }
protected:
    std::vector<ObjectGuid> Calculate() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        if (!bot || !bot->IsInWorld() || !bot->IsAlive() || bot->IsBeingTeleported()) return {};
        std::set<ObjectGuid> targets;
        auto add = [&](Player& member)
        {
            if (!member.IsInWorld() || !member.IsAlive() || member.IsBeingTeleported() ||
                member.GetMap() != bot->GetMap() || bot->GetExactDist2d(&member) > 100.0f) return;
            for (auto const& [guid, ref] : member.GetThreatManager().GetThreatenedByMeList())
            {
                Creature* target = ref && ref->GetOwner() ? ref->GetOwner()->ToCreature() : nullptr;
                if (target && member.IsValidAttackTarget(target) && member.GetExactDist2d(target) < 100.0f &&
                    ValidEngagedCreature(*bot, botAI->GetController(), *target))
                    targets.insert(guid);
            }
        };
        add(*bot);
        if (Group* group = bot->GetGroup())
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                if (Player* member = ref->GetSource(); member && member != bot) add(*member);
        return {targets.begin(), targets.end()}; // cached results never contain Unit pointers
    }
};
class BalanceValue final : public CalculatedValue<uint8>
{
public:
    explicit BalanceValue(PlayerbotAI* ai) : CalculatedValue(ai, "balance") { }
protected:
    uint8 Calculate() override
    {
        Player* bot = botAI ? botAI->GetBot() : nullptr;
        AiObjectContext* context = botAI ? botAI->GetAiObjectContext() : nullptr;
        auto* attackers = context ? context->GetValue<std::vector<ObjectGuid>>("attackers") : nullptr;
        if (!bot || !bot->IsAlive() || !attackers) return 0;
        float playerLevels = 0.0f, enemyLevels = 0.0f;
        Group* group = bot->GetGroup();
        if (group)
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                if (Player* member = ref->GetSource(); member && member->IsInWorld() && member->IsAlive() &&
                    !member->IsBeingTeleported() && member->GetMap() == bot->GetMap())
                    playerLevels += float(member->getLevel());
        for (ObjectGuid guid : attackers->Get())
            if (Creature* target = ObjectAccessor::GetCreature(*bot, guid); target && target->IsAlive())
                enemyLevels += PlayerbotCombatBalance::EnemyWeight(target->getLevel(), target->GetCreatureTemplate()->rank);
        return PlayerbotCombatBalance::Percent(playerLevels, group ? group->GetMembersCount() : 0, enemyLevels);
    }
};
class ThreatValue final : public CalculatedValue<uint8>, public Qualified
{
public:
    explicit ThreatValue(PlayerbotAI* ai) : CalculatedValue(ai, "threat") { }
protected:
    uint8 Calculate() override
    {
        if (!botAI || (!qualifier.empty() && qualifier != "current target")) return 0;
        Player* bot = botAI->GetBot();
        Creature* target = botAI->GetCurrentTarget();
        Group* group = bot ? bot->GetGroup() : nullptr;
        if (!bot || !bot->IsAlive() || !group || !target || !target->IsAlive() ||
            target->IsControlledByPlayer() || target->GetMap() != bot->GetMap() || !target->CanHaveThreatList()) return 0;
        auto const& manager = target->GetThreatManager();
        float tankThreat = 0.0f;
        bool hasTank = false;
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (member && member != bot && member->IsAlive() && member->GetMap() == bot->GetMap() && IsImplementedTank(*member))
            {
                hasTank = true; // human tanks also participate, as in the donor
                tankThreat = std::max(tankThreat, manager.GetThreat(member));
            }
        }
        MovementGeneratorType movement = target->GetMotionMaster()->GetCurrentMovementGeneratorType();
        bool fleeing = movement == FLEEING_MOTION_TYPE || movement == TIMED_FLEEING_MOTION_TYPE;
        return PlayerbotThreat::Percent(manager.GetThreat(bot), tankThreat, hasTank, target->IsInCombat(), fleeing);
    }
};
}
void PlayerbotCombatValues::AddContexts(SharedNamedObjectContextList<UntypedValue>& values)
{
    PlayerbotTargetSelection::AddContexts(values);
    auto* factory = new NamedObjectContext<UntypedValue>();
    factory->creators["estimated group dps"] = [](PlayerbotAI* ai) { return new EstimatedGroupDpsValue(ai); };
    factory->creators["estimated lifetime"] = [](PlayerbotAI* ai) { return new EstimatedLifetimeValue(ai); };
    factory->creators["threat"] = [](PlayerbotAI* ai) { return new ThreatValue(ai); };
    factory->creators["attackers"] = [](PlayerbotAI* ai) { return new AttackersValue(ai); };
    factory->creators["balance"] = [](PlayerbotAI* ai) { return new BalanceValue(ai); };
    factory->creators["neglect threat"] = [](PlayerbotAI* ai) { return new PlayerbotThreat::NeglectThreatResetValue(ai); };
    values.Add(factory);
}
