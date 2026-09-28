/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */
// Ported from mod-playerbots 8827dd6fcbb2bb25988787a40f06fc93daf8e02d.

#ifndef PLAYERBOTS_EVENT_H
#define PLAYERBOTS_EVENT_H

#include "ObjectGuid.h"
#include "WorldPacket.h"

class Player;

class Event
{
public:
    Event(Event const& other) = default;
    Event& operator=(Event const& other) = default;
    Event() {}
    Event(std::string const source) : source(source) {}
    Event(std::string const source, std::string const param, Player* owner = nullptr);
    Event(std::string const source, WorldPacket& packet, Player* owner = nullptr);
    Event(std::string const source, ObjectGuid object, Player* owner = nullptr);

    // GUID forms are useful at queued event boundaries. They retain identity,
    // never ownership of a Player or a borrowed packet buffer.
    Event(std::string const source, std::string const param, ObjectGuid ownerGuid);
    Event(std::string const source, WorldPacket const& packet, ObjectGuid ownerGuid);
    Event(std::string const source, ObjectGuid object, ObjectGuid ownerGuid);
    virtual ~Event() {}

    std::string const GetSource() { return source; }
    std::string const getParam() { return param; }
    WorldPacket& getPacket() { return packet; }
    ObjectGuid getObject();
    // Only use the returned pointer within the current call, in the proper
    // execution context. Security/ownership authorization is not supplied here.
    Player* getOwner();
    ObjectGuid getOwnerGuid() const { return ownerGuid; }
    bool operator!() const { return source.empty(); }

protected:
    std::string source;
    std::string param;
    WorldPacket packet;
    ObjectGuid ownerGuid;
};

#endif
