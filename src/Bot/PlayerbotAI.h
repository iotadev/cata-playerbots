/*
 * This file is part of the Cataclysm Playerbots port. See AUTHORS.md.
 * Released under GNU GPL v2 or any later version.
 */
#ifndef PLAYERBOTS_PLAYERBOTAI_H
#define PLAYERBOTS_PLAYERBOTAI_H

#include "ObjectGuid.h"
#include "../Ai/Base/PlayerbotCorpseLoot.h"

class Creature;
class Player;
class WorldSession;
class AiObjectContext;
class Engine;

// Session-bound Cata adapter for the donor engine's PlayerbotAI* interfaces.
// It never owns or retains a Player pointer. Accessors are map-thread only.
class PlayerbotAI
{
public:
    explicit PlayerbotAI(WorldSession& session) : session(session) { }
    Player* GetBot() const;
    Player* GetController() const;
    Creature* GetCurrentTarget() const;
    // Borrowed engine context; map-thread only, bound/cleared by its lifetime.
    AiObjectContext* GetAiObjectContext() const { return context; }
    void SetAiObjectContext(AiObjectContext* value) { context = value; }
    // Borrowed decision engine; map-thread only, bound/cleared by Engine lifetime.
    Engine const* GetDecisionEngine() const { return decisionEngine; }
    void SetDecisionEngine(Engine const* value) { decisionEngine = value; }
    void SetController(ObjectGuid guid)
    {
        if (controllerGuid != guid)
            lootCandidates.Clear();
        controllerGuid = guid;
    }
    void ClearController() { controllerGuid.Clear(); lootCandidates.Clear(); }
    void SetCurrentTarget(ObjectGuid guid) { targetGuid = guid; }
    void ClearCurrentTarget() { targetGuid.Clear(); }
    // Map-owned rest state contains only a spell identity, never an item/player pointer.
    void BeginRest(uint32 spellId) { restSpellId = spellId; }
    uint32 GetRestSpellId() const { return restSpellId; }
    void ClearRest() { restSpellId = 0; }
    PlayerbotCorpseLoot::Mailbox& LootRequests() { return lootRequests; }
    PlayerbotCorpseLoot::AttemptHistory& LootAttempts() { return lootAttempts; }
    void RecordDefeatedCreature(ObjectGuid guid, uint32 now) { defeatedCreature = guid; lootCandidates.Add(guid, now); }
    ObjectGuid GetDefeatedCreature() const { return defeatedCreature; }
    PlayerbotCorpseLoot::Candidates& LootCandidates() { return lootCandidates; }
    PlayerbotCorpseLoot::Pursuit& LootPursuit() { return lootPursuit; }

private:
    WorldSession& session;
    AiObjectContext* context = nullptr;
    Engine const* decisionEngine = nullptr;
    ObjectGuid controllerGuid;
    ObjectGuid targetGuid;
    uint32 restSpellId = 0;
    PlayerbotCorpseLoot::Mailbox lootRequests;
    PlayerbotCorpseLoot::AttemptHistory lootAttempts; // map-owned
    ObjectGuid defeatedCreature; // map-owned, no retained creature pointer
    PlayerbotCorpseLoot::Candidates lootCandidates; // map-owned
    PlayerbotCorpseLoot::Pursuit lootPursuit; // map-owned
};

#endif
