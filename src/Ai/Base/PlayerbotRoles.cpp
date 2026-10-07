/* GPL v2 or later. Donor provenance and Cata adaptations are in PORTING.md. */
#include "PlayerbotRoles.h"
#include "WorldSession.h"
#include "Group.h"
uint32 PlayerbotRoles::Mask(Player const& player, bool bySpec)
{
    uint32 configured = !bySpec && player.GetSession() ? player.GetSession()->GetPlayerbotStrategyRoleMask() : 0;
    return ChooseMask(configured, SpecMask(player.getClass(),
        player.GetPrimaryTalentTree(player.GetActiveSpec()), player.GetShapeshiftForm()));
}
bool PlayerbotRoles::IsExplicitMainTank(Player const& player)
{
    Group const* group = player.GetGroup();
    if (!group || !group->IsMember(player.GetGUID())) return false;
    // Donor checks the first assigned slot; an offline assignment is not
    // silently replaced. Native group code owns assignment uniqueness.
    for (auto const& slot : group->GetMemberSlots())
        if (slot.flags & MEMBER_FLAG_MAINTANK)
            return slot.guid == player.GetGUID();
    return false;
}
uint32 PlayerbotRoles::GroupTankCount(Player const& player)
{
    Group const* group = player.GetGroup();
    if (!group || !group->IsMember(player.GetGUID())) return 0;
    uint32 count = 0;
    for (GroupReference const* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->IsInWorld() && member->IsAlive() &&
            !member->IsBeingTeleported() && member->GetMap() == player.GetMap() && IsTank(*member))
            ++count;
    }
    return count;
}
ObjectGuid PlayerbotRoles::MainTankGuid(Player const& player)
{
    Group const* group = player.GetGroup();
    if (!group) return player.IsAlive() && IsTank(player) ? player.GetGUID() : ObjectGuid::Empty;
    if (!group->IsMember(player.GetGUID())) return ObjectGuid::Empty;
    MainTankSelection<ObjectGuid> selection;
    for (auto const& slot : group->GetMemberSlots())
        selection.Observe(slot.guid, (slot.flags & MEMBER_FLAG_MAINTANK) != 0, false);
    // Explicit assignment is authoritative, including an unavailable assignee.
    // Do not silently promote an off-tank when the main tank dies or disconnects.
    if (!selection.Assigned.IsEmpty()) return selection.Get();
    for (GroupReference const* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (member && member->IsInWorld() && member->IsAlive() &&
            !member->IsBeingTeleported() && member->GetMap() == player.GetMap())
            selection.Observe(member->GetGUID(), false, IsTank(*member));
    }
    return selection.Get();
}
bool PlayerbotRoles::IsMainTank(Player const& player)
{
    return player.GetGroup() ? MainTankGuid(player) == player.GetGUID() : IsTank(player);
}
