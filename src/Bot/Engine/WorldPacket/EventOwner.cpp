/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */
// Ported from mod-playerbots 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.

// Cata adapter: queued events store identity; resolve the live owner at use.
#include "Event.h"
#include "ObjectAccessor.h"
#include "Player.h"

Event::Event(std::string const source, std::string const param, Player* owner)
    : Event(source, param, owner ? owner->GetGUID() : ObjectGuid())
{
}

Event::Event(std::string const source, WorldPacket& packet, Player* owner)
    : Event(source, packet, owner ? owner->GetGUID() : ObjectGuid())
{
}

Event::Event(std::string const source, ObjectGuid object, Player* owner)
    : Event(source, object, owner ? owner->GetGUID() : ObjectGuid())
{
}

Player* Event::getOwner()
{
    return ownerGuid.IsEmpty() ? nullptr : ObjectAccessor::FindPlayer(ownerGuid);
}
