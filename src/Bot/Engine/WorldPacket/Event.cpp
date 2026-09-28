/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */
// Ported from mod-playerbots 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.

// Payload operations need only the native packet/GUID types, not PlayerbotAI.
#include "Event.h"

Event::Event(std::string const source, std::string const param, ObjectGuid ownerGuid)
    : source(source), param(param), ownerGuid(ownerGuid)
{
}

Event::Event(std::string const source, WorldPacket const& packet, ObjectGuid ownerGuid)
    : source(source), packet(packet), ownerGuid(ownerGuid)
{
}

Event::Event(std::string const source, ObjectGuid object, ObjectGuid ownerGuid)
    : source(source), ownerGuid(ownerGuid)
{
    // Matches Cata ObjectGuid.cpp's native uncompressed GUID serialization.
    packet << uint64(object.GetRawValue());
}

ObjectGuid Event::getObject()
{
    if (packet.size() < sizeof(uint64))
        return ObjectGuid();

    WorldPacket copy(packet);
    copy.rpos(0);
    return ObjectGuid(copy.read<uint64>());
}
